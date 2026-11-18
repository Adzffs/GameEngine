#include "TestSupport.h"
#include "../src/Content/Edit/StarterWorldDraftFile.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace
{
    struct TempDirectory
    {
        std::filesystem::path path = std::filesystem::temp_directory_path() /
            ("embervale-draft-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        TempDirectory() { std::filesystem::create_directory(path); }
        ~TempDirectory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    };

    void Write(const std::filesystem::path &path, const std::string &bytes)
    {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file << bytes;
        if (!file) throw std::runtime_error("could not prepare draft test input");
    }

    template<typename Operation>
    bool Reject(Operation operation)
    {
        try { operation(); }
        catch (const std::runtime_error &) { return true; }
        return false;
    }
}

int main()
{
    TestContext test;
    using namespace StarterWorldDraftFile;
    test.ExpectEqual(Digest(""), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), "SHA-256 empty vector");
    test.ExpectEqual(Digest("abc"), std::string("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "SHA-256 abc vector");
    test.ExpectEqual(Digest(std::string(1000000, 'a')), std::string("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"), "SHA-256 large multi-block vector");
    test.Expect(ValidDigest(Digest("abc")), "canonical hash accepted");
    test.Expect(!ValidDigest("ABC") && !ValidDigest(std::string(64, 'g')),
        "invalid digest formats rejected");

    TempDirectory temp;
    const auto source = temp.path / "source.json";
    const auto target = temp.path / "revision.json";
    Write(source, "hello world\n");
    const std::string baseline = Digest(Read(source));
    PublishNew(source, baseline, "new canonical JSON\n", target);
    test.ExpectEqual(Read(source), std::string("hello world\n"), "draft writer never modifies source");
    test.ExpectEqual(Read(target), std::string("new canonical JSON\n"), "published draft bytes are exact");
    test.Expect(!std::filesystem::exists(source.string() + ".embervale.lock"),
        "successful publication releases cooperative source lock");
    test.Expect(Reject([&]{ PublishNew(source, baseline, "another", target); }),
        "cannot overwrite existing draft");
    test.ExpectEqual(Read(target), std::string("new canonical JSON\n"),
        "rejected overwrite leaves original draft intact");
    test.Expect(Reject([&]{ PublishNew(source, std::string(64, '0'), "x", temp.path / "next.json"); }),
        "stale base hash rejects candidate before publication");
    test.Expect(!std::filesystem::exists(temp.path / "next.json"),
        "stale hash did not create a draft");
    test.Expect(Reject([&]{ PublishNew(source, baseline, "x", temp.path / "sub" / "next.json"); }),
        "cross-directory write rejected");
    std::filesystem::create_directory(source.string() + ".embervale.lock");
    test.Expect(Reject([&]{ PublishNew(source, baseline, "x", temp.path / "locked.json"); }),
        "held source lock rejects concurrent writer");
    std::filesystem::remove(source.string() + ".embervale.lock");
    std::error_code symlinkError;
    std::filesystem::create_symlink(source, temp.path / "link.json", symlinkError);
    if (!symlinkError)
        test.Expect(Reject([&]{ (void)Read(temp.path / "link.json"); }), "symlink input rejected where supported");
    test.Expect(Reject([&]{ (void)Read(temp.path); }), "directory input rejected");
    Write(source, "changed\n");
    test.Expect(Reject([&]{ PublishNew(source, baseline, "x", temp.path / "stale.json"); }),
        "source bytes changing after inspection produces conflict");
    test.Expect(!std::filesystem::exists(temp.path / "stale.json"),
        "conflicting change leaves draft absent");
    return test.Finish();
}
