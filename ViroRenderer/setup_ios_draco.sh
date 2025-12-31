#!/bin/bash

# Setup Draco for iOS build by creating a separate, cleaned copy.
# This allows the original 'draco' directory to remain intact for Android builds.

DRACO_SRC="./draco"
DRACO_IOS_DST="./draco_ios"

if [ ! -d "$DRACO_SRC" ]; then
    echo "Error: Draco source directory not found at $DRACO_SRC"
    echo "Please clone draco first: git clone https://github.com/google/draco.git"
    exit 1
fi

if [ ! -f "$DRACO_SRC/CMakeLists.txt" ]; then
    echo "Warning: $DRACO_SRC/CMakeLists.txt is missing."
    echo "It seems 'draco' is already cleaned or corrupted."
    echo "For Android builds, you should restore 'draco' to its original state (git checkout)."
fi

echo "Creating $DRACO_IOS_DST from $DRACO_SRC..."
rm -rf "$DRACO_IOS_DST"
cp -R "$DRACO_SRC" "$DRACO_IOS_DST"

echo "Cleaning $DRACO_IOS_DST for iOS integration..."
DRACO_PATH="$DRACO_IOS_DST"

# 1. Remove Test Files
find "$DRACO_PATH" -type f -name "*_test.cc" -delete
find "$DRACO_PATH" -type f -name "*_test.h" -delete
find "$DRACO_PATH" -type f -name "test_utils.*" -delete
find "$DRACO_PATH" -type f -name "draco_test_base.h" -delete
find "$DRACO_PATH" -type f -name "draco_test_utils.h" -delete
find "$DRACO_PATH" -type f -name "draco_test_utils.cc" -delete

# 2. Remove Fuzzers
find "$DRACO_PATH" -type f -name "*_fuzzer.cc" -delete

# 3. Remove Tools
rm -rf "$DRACO_PATH/src/draco/tools"
rm -rf "$DRACO_PATH/src/draco/javascript"
rm -rf "$DRACO_PATH/src/draco/unity"
rm -rf "$DRACO_PATH/src/draco/maya"
rm -rf "$DRACO_PATH/examples"

# 4. Remove Unused Modules
rm -rf "$DRACO_PATH/src/draco/scene"
rm -rf "$DRACO_PATH/src/draco/animation"
rm -rf "$DRACO_PATH/src/draco/io"

# 5. Remove CMake/Build configs
find "$DRACO_PATH" -name "CMakeLists.txt" -delete
find "$DRACO_PATH" -name "*.cmake" -delete
rm -rf "$DRACO_PATH/cmake"

# 6. Generate draco_features.h
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

echo "----------------------------------------------------------------"
echo "Draco for iOS is ready in '$DRACO_IOS_DST'."
echo "IMPORTANT: Please update your Xcode project to reference files"
echo "from '$DRACO_IOS_DST' instead of '$DRACO_SRC'."
echo "----------------------------------------------------------------"
