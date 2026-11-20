#include "StarterWorldDraftFile.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef GAMEENGINE_DRAFT_CRASH_TEST
// Only the standalone crash-test executable defines this symbol. Production
// libraries contain no pause hooks, environment switches or fault injection.
void GameEngineDraftTestPause(std::string_view phase);
#endif

namespace StarterWorldDraftFile
{
    namespace
    {
        [[noreturn]] void Fail(const std::string &message)
        {
            throw std::runtime_error("starter world draft: " + message);
        }

        constexpr std::array<std::uint32_t, 64> K{
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};

        constexpr std::uint32_t Right(std::uint32_t x, unsigned amount)
        {
            return (x >> amount) | (x << (32 - amount));
        }

        class OwnedDirectory
        {
        public:
            explicit OwnedDirectory(std::filesystem::path directory)
                : path_(std::move(directory))
            {
                std::error_code error;
                if (!std::filesystem::create_directory(path_, error) || error)
                    Fail("cannot reserve directory: " + path_.string());
            }
            OwnedDirectory(const OwnedDirectory &) = delete;
            OwnedDirectory &operator=(const OwnedDirectory &) = delete;
            ~OwnedDirectory()
            {
                std::error_code ignored;
                std::filesystem::remove_all(path_, ignored);
            }
            const std::filesystem::path &Path() const { return path_; }
        private:
            std::filesystem::path path_;
        };

        class SourceLock
        {
        public:
            explicit SourceLock(const std::filesystem::path &source)
            {
                auto legacy = source; legacy += ".embervale.lock";
                std::error_code error;
                if (std::filesystem::exists(legacy, error) || error)
                    Fail("legacy save lock requires inspection; recovery can export a separate copy");
                auto path = source; path += ".embervale.writer.lock";
                const auto status = std::filesystem::symlink_status(path, error);
                if ((!error && std::filesystem::exists(status) && !std::filesystem::is_regular_file(status)) ||
                    (error && error != std::errc::no_such_file_or_directory))
                    Fail("writer lock must be a regular non-symlink file");
#ifdef _WIN32
                handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
                if (handle == INVALID_HANDLE_VALUE) Fail("writer is active or lock file is unavailable");
                BY_HANDLE_FILE_INFORMATION info{};
                if (!GetFileInformationByHandle(handle, &info) ||
                    (info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)))
                { CloseHandle(handle); handle = INVALID_HANDLE_VALUE; Fail("invalid writer lock file"); }
#else
                descriptor = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW, 0600);
                struct stat info{};
                if (descriptor < 0 || fstat(descriptor, &info) != 0 || !S_ISREG(info.st_mode) ||
                    flock(descriptor, LOCK_EX | LOCK_NB) != 0)
                {
                    if (descriptor >= 0) close(descriptor);
                    descriptor = -1;
                    Fail("writer is active or lock file is unavailable");
                }
#endif
            }
            SourceLock(const SourceLock &) = delete;
            SourceLock &operator=(const SourceLock &) = delete;
            ~SourceLock()
            {
#ifdef _WIN32
                if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
#else
                if (descriptor >= 0) close(descriptor);
#endif
                // Keep this inode so cooperating waiters cannot split locks.
            }
        private:
#ifdef _WIN32
            HANDLE handle = INVALID_HANDLE_VALUE;
#else
            int descriptor = -1;
#endif
        };

        std::filesystem::path StagingDirectory(const std::filesystem::path &parent)
        {
            std::random_device random;
            const auto time = std::chrono::steady_clock::now().time_since_epoch().count();
            for (int i = 0; i < 16; ++i)
            {
                const auto candidate = parent / (".embervale-draft-" +
                    std::to_string(time) + "-" + std::to_string(random()) +
                    "-" + std::to_string(i));
                std::error_code error;
                if (std::filesystem::exists(candidate, error) || error)
                    continue;
                return candidate;
            }
            Fail("could not allocate staging directory");
        }

#ifdef _WIN32
        void PublishExclusiveWindows(const std::filesystem::path &destination,
            std::string_view candidateBytes)
        {
            const HANDLE file = CreateFileW(destination.c_str(), GENERIC_WRITE, 0,
                nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE)
            {
                const auto error = GetLastError();
                if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS)
                    Fail("output already exists");
                Fail("cannot create new draft: Windows error " + std::to_string(error));
            }

            bool complete = false;
            std::size_t offset = 0;
            while (offset < candidateBytes.size())
            {
                const auto remaining = candidateBytes.size() - offset;
                auto requested = static_cast<DWORD>(remaining);
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
                if (offset == 0) requested = static_cast<DWORD>((remaining + 1) / 2);
#endif
                DWORD written = 0;
                if (!WriteFile(file, candidateBytes.data() + offset, requested,
                        &written, nullptr) || written == 0)
                    break;
                offset += written;
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
                if (offset < candidateBytes.size()) GameEngineDraftTestPause("writing");
#endif
            }
            complete = offset == candidateBytes.size() && FlushFileBuffers(file);
            const bool closed = CloseHandle(file) != 0;
            if (!complete || !closed)
            {
                // CREATE_NEW proves this process owns the destination entry.
                // Remove only that incomplete entry; never touch a pre-existing file.
                DeleteFileW(destination.c_str());
                Fail("new draft write failed");
            }
        }
#endif
    }

    bool ValidDigest(std::string_view digest)
    {
        if (digest.size() != 64) return false;
        for (const char character : digest)
            if (!((character >= '0' && character <= '9') ||
                  (character >= 'a' && character <= 'f')))
                return false;
        return true;
    }

    std::string Digest(std::string_view bytes)
    {
        // SHA-256 (FIPS 180-4), operating in 64-byte blocks without allocating
        // an expanded copy of the up-to-2-MiB input.
        std::array<std::uint32_t, 8> h{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
            0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
        const std::uint64_t bits = static_cast<std::uint64_t>(bytes.size()) * 8;
        const std::size_t padded = ((bytes.size() + 9 + 63) / 64) * 64;
        for (std::size_t offset = 0; offset < padded; offset += 64)
        {
            std::array<std::uint32_t, 64> w{};
            for (std::size_t i = 0; i < 64; ++i)
            {
                const std::size_t index = offset + i;
                std::uint8_t byte = 0;
                if (index < bytes.size())
                    byte = static_cast<std::uint8_t>(bytes[index]);
                else if (index == bytes.size())
                    byte = 0x80;
                else if (index >= padded - 8)
                    byte = static_cast<std::uint8_t>(bits >> ((padded - index - 1) * 8));
                w[i / 4] |= static_cast<std::uint32_t>(byte) << ((3 - i % 4) * 8);
            }
            for (std::size_t i = 16; i < w.size(); ++i)
            {
                const auto s0 = Right(w[i - 15], 7) ^ Right(w[i - 15], 18) ^ (w[i - 15] >> 3);
                const auto s1 = Right(w[i - 2], 17) ^ Right(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }
            auto [a, b, c, d, e, f, g, hh] =
                std::tuple{h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7]};
            for (std::size_t i = 0; i < 64; ++i)
            {
                const auto s1 = Right(e, 6) ^ Right(e, 11) ^ Right(e, 25);
                const auto choose = (e & f) ^ (~e & g);
                const auto t1 = hh + s1 + choose + K[i] + w[i];
                const auto s0 = Right(a, 2) ^ Right(a, 13) ^ Right(a, 22);
                const auto majority = (a & b) ^ (a & c) ^ (b & c);
                const auto t2 = s0 + majority;
                hh = g; g = f; f = e; e = d + t1;
                d = c; c = b; b = a; a = t1 + t2;
            }
            const std::array<std::uint32_t, 8> state{a,b,c,d,e,f,g,hh};
            for (std::size_t i = 0; i < h.size(); ++i) h[i] += state[i];
        }
        constexpr char Hex[] = "0123456789abcdef";
        std::string output(64, '0');
        for (std::size_t i = 0; i < h.size(); ++i)
            for (std::size_t j = 0; j < 8; ++j)
                output[8 * i + j] = Hex[(h[i] >> (28 - 4 * j)) & 0xf];
        return output;
    }

    std::string Read(const std::filesystem::path &path)
    {
        std::error_code error;
        const auto status = std::filesystem::symlink_status(path, error);
        if (error || !std::filesystem::is_regular_file(status))
            Fail("source must be a regular non-symlink file: " + path.string());
        const auto size = std::filesystem::file_size(path, error);
        if (error || size > MaximumBytes)
            Fail("source is unreadable or exceeds 2 MiB: " + path.string());
        std::ifstream input(path, std::ios::binary);
        if (!input) Fail("cannot open source: " + path.string());
        std::string bytes;
        bytes.reserve(static_cast<std::size_t>(size));
        std::array<char, 4096> chunk{};
        while (input)
        {
            input.read(chunk.data(), static_cast<std::streamsize>(chunk.size()));
            const auto count = static_cast<std::size_t>(input.gcount());
            if (count > MaximumBytes - bytes.size()) Fail("source grew beyond 2 MiB");
            bytes.append(chunk.data(), count);
        }
        if (input.bad() || !input.eof()) Fail("source read failed");
        return bytes;
    }

    static void Publish(const std::filesystem::path &source,
        std::string_view expectedHash, std::string_view candidateBytes,
        const std::filesystem::path &destination, bool recoveryCopy)
    {
        if (!ValidDigest(expectedHash)) Fail("expected SHA-256 must be 64 lowercase hex characters");
        if (candidateBytes.empty() || candidateBytes.size() > MaximumBytes)
            Fail("candidate must be nonempty and no larger than 2 MiB");
        if (source.empty() || destination.empty() || destination.filename().empty())
            Fail("explicit source and output filename required");
        std::error_code pathError;
        const auto sourceParent = std::filesystem::weakly_canonical(
            source.parent_path().empty() ? "." : source.parent_path(), pathError);
        if (pathError) Fail("cannot resolve source directory");
        const auto targetParent = std::filesystem::weakly_canonical(
            destination.parent_path().empty() ? "." : destination.parent_path(), pathError);
        if (pathError || (!recoveryCopy && sourceParent != targetParent))
            Fail("new draft must be a sibling of the source file");
        if (recoveryCopy && destination.extension() != ".json")
            Fail("recovery copy requires a new .json filename");
        std::error_code error;
        if (!std::filesystem::is_directory(destination.parent_path().empty() ? "." :
            destination.parent_path(), error) || error)
            Fail("output parent directory must already exist");
        if (std::filesystem::exists(destination, error) || error)
            Fail("output already exists or cannot be checked");
        SourceLock lock(source);
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
        GameEngineDraftTestPause("locked");
#endif
        const auto current = Read(source);
        if (Digest(current) != expectedHash)
            Fail("CONTENT_CONFLICT: source bytes no longer match expected SHA-256");
        if (std::filesystem::exists(destination, error) || error)
            Fail("output already exists or cannot be checked");
        const auto directory = destination.parent_path().empty() ?
            std::filesystem::path{"."} : destination.parent_path();
        OwnedDirectory stage(StagingDirectory(directory));
        const auto temporary = stage.Path() / "snapshot.json";
#ifdef _WIN32
        PublishExclusiveWindows(temporary, candidateBytes);
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
        GameEngineDraftTestPause("staged");
#endif
        // Same-directory staging avoids a cross-volume copy. MoveFileW fails
        // if the selected name exists; no partially written target is exposed.
        if (!MoveFileW(temporary.c_str(), destination.c_str()))
            Fail("cannot publish complete draft: Windows error " + std::to_string(GetLastError()));
#else
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            if (!out) Fail("cannot create temporary draft");
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
            const auto half = candidateBytes.size() / 2;
            out.write(candidateBytes.data(), static_cast<std::streamsize>(half));
            out.flush();
            GameEngineDraftTestPause("writing");
            out.write(candidateBytes.data() + half, static_cast<std::streamsize>(candidateBytes.size() - half));
#else
            out.write(candidateBytes.data(), static_cast<std::streamsize>(candidateBytes.size()));
#endif
            out.flush();
            if (!out) Fail("temporary draft write failed");
            out.close();
            if (!out) Fail("temporary draft close failed");
        }
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
        GameEngineDraftTestPause("staged");
#endif
        // Create the final directory entry exclusively, in one filesystem
        // operation. A competing writer's output is never overwritten, even
        // if it appeared after the earlier existence check.
        std::filesystem::create_hard_link(temporary, destination, error);
        if (error)
            Fail("cannot create new draft (target may already exist or filesystem lacks hard links): " +
                error.message());
#endif
#ifdef GAMEENGINE_DRAFT_CRASH_TEST
        GameEngineDraftTestPause("published");
#endif
    }

    void PublishNew(const std::filesystem::path &source, std::string_view expectedHash,
        std::string_view candidateBytes, const std::filesystem::path &destination)
    { Publish(source, expectedHash, candidateBytes, destination, false); }

    void PublishRecoveryCopy(const std::filesystem::path &source, std::string_view expectedHash,
        std::string_view candidateBytes, const std::filesystem::path &destination)
    { Publish(source, expectedHash, candidateBytes, destination, true); }
}
