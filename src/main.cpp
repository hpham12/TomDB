//
// Created by Hieu Pham on 9/12/26.
//

#include <stdexcept>

inline constexpr std::string_view DEFAULT_METADATA_FILE_PATH = "/.tomdb/metadata";

bool metadataFileExists(std::string_view filePath) {
    // TODO: implement
}

void createMetadataFile(std::string_view filePath) {
    // TODO: implement
}

int main() {
    if (!metadataFileExists(DEFAULT_METADATA_FILE_PATH)) {
        createMetadataFile(DEFAULT_METADATA_FILE_PATH);
    }

    // TODO: Read metadata file, then invoke Metadata#deserialize

    // TODO: Create an infinite loop that takes input from user, then invoke QueryProcessor for each query
}
