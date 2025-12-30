#!/bin/bash

# Define the Draco path
DRACO_PATH="./draco"

if [ ! -d "$DRACO_PATH" ]; then
    echo "Error: Draco directory not found at $DRACO_PATH"
    echo "Please clone draco first: git clone https://github.com/google/draco.git"
    exit 1
fi

echo "Cleaning Draco directory for iOS integration..."

# 1. Remove Test Files
echo "Removing test files (*_test.cc, *_test.h)..."
find "$DRACO_PATH" -type f -name "*_test.cc" -delete
find "$DRACO_PATH" -type f -name "*_test.h" -delete
find "$DRACO_PATH" -type f -name "test_utils.*" -delete
find "$DRACO_PATH" -type f -name "draco_test_base.h" -delete
find "$DRACO_PATH" -type f -name "draco_test_utils.h" -delete
find "$DRACO_PATH" -type f -name "draco_test_utils.cc" -delete

# 2. Remove Fuzzers
echo "Removing fuzzers..."
find "$DRACO_PATH" -type f -name "*_fuzzer.cc" -delete

# 3. Remove Tools (CLI applications that contain 'main' functions)
echo "Removing tools and apps..."
rm -rf "$DRACO_PATH/src/draco/tools"
rm -rf "$DRACO_PATH/src/draco/javascript"
rm -rf "$DRACO_PATH/src/draco/unity"
rm -rf "$DRACO_PATH/src/draco/maya"
rm -rf "$DRACO_PATH/examples"

# 4. Remove Unused Modules (Causes iOS build errors)
# Note: kept texture and metadata as they appear to be dependencies of point_cloud
echo "Removing unused modules (scene, animation, io)..."
rm -rf "$DRACO_PATH/src/draco/scene"
rm -rf "$DRACO_PATH/src/draco/animation"
rm -rf "$DRACO_PATH/src/draco/io"

# 5. Remove CMake/Build configs (optional, but keeps visual noise down in Xcode)
echo "Removing build configuration files..."
find "$DRACO_PATH" -name "CMakeLists.txt" -delete
find "$DRACO_PATH" -name "*.cmake" -delete
rm -rf "$DRACO_PATH/cmake"

# 6. Generate draco_features.h (Required because we aren't running CMake)
echo "Generating draco_features.h..."
FEATURES_FILE="$DRACO_PATH/src/draco/draco_features.h"
cat <<EOF > "$FEATURES_FILE"
#ifndef DRACO_FEATURES_H_
#define DRACO_FEATURES_H_

#define DRACO_POINT_CLOUD_COMPRESSION_SUPPORTED
#define DRACO_MESH_COMPRESSION_SUPPORTED
#define DRACO_NORMAL_ENCODING_SUPPORTED
#define DRACO_STANDARD_EDGEBREAKER_SUPPORTED
#define DRACO_PREDICTIVE_EDGEBREAKER_SUPPORTED
#define DRACO_BACKWARDS_COMPATIBILITY_SUPPORTED

#endif  // DRACO_FEATURES_H_
EOF

echo "Done! The '$DRACO_PATH' directory is now ready to be added to Xcode."
echo "Drag the '$DRACO_PATH/src' folder into your Xcode project."
