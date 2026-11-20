#include "../Content/Publication/WorldPublication.h"
#include "../Content/Edit/StarterWorldDraftFile.h"
#include <iostream>
#include <string_view>

int main(int argc, char **argv)
{
    using namespace WorldPublication;
    const auto usage = [] {
        std::cout << "Usage:\n"
            "  EmbervaleWorldPublish --validate <draft.json>\n"
            "  EmbervaleWorldPublish --publish <draft.json> --receipt <receipt.json> --library <directory> --revision <name>\n"
            "  EmbervaleWorldPublish --inspect <version/manifest.json>\n"
            "Validation and inspection are read-only. Publishing creates a new immutable world/manifest pair.\n";
    };
    if (argc == 2 && std::string_view(argv[1]) == "--help") { usage(); return 0; }
    try
    {
        if (argc == 3 && std::string_view(argv[1]) == "--validate")
            std::cout << SerializeReceipt(ValidateDraft(argv[2]));
        else if (argc == 3 && std::string_view(argv[1]) == "--inspect")
            std::cout << SerializeManifest(Load(argv[2]).manifest);
        else if (argc == 9 && std::string_view(argv[1]) == "--publish" &&
            std::string_view(argv[3]) == "--receipt" && std::string_view(argv[5]) == "--library" &&
            std::string_view(argv[7]) == "--revision")
            std::cout << SerializeManifest(Publish(argv[2],
                ParseReceipt(StarterWorldDraftFile::Read(argv[4], MaximumMetadataBytes)), argv[6], argv[8]));
        else { usage(); return 2; }
        return 0;
    }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
