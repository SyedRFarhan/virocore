//
//  VRODracoMeshLoader.cpp
//  ViroRenderer
//
//  Copyright © 2018 Viro Media. All rights reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining
//  a copy of this software and associated documentation files (the
//  "Software"), to deal in the Software without restriction, including
//  without limitation the rights to use, copy, modify, merge, publish,
//  distribute, sublicense, and/or sell copies of the Software, and to
//  permit persons to whom the Software is furnished to do so, subject to
//  the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
//  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
//  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
//  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
//  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "VRODracoMeshLoader.h"
#include "VROByteBuffer.h"
#include "VROGeometryElement.h"
#include "VROGeometrySource.h"
#include "VROLog.h"
#include "draco/compression/decode.h"
#include "draco/mesh/mesh.h"

bool VRODracoMeshLoader::decodeDracoData(
    const std::vector<char> &data,
    const std::map<std::string, int> &attributeTypeMap,
    std::vector<std::shared_ptr<VROGeometrySource>> &sourcesOut,
    std::vector<std::shared_ptr<VROGeometryElement>> &elementsOut) {

  draco::DecoderBuffer buffer;
  buffer.Init(data.data(), data.size());

  draco::Decoder decoder;
  auto statusor = decoder.DecodeMeshFromBuffer(&buffer);
  if (!statusor.ok()) {
    pwarn("Failed to decode Draco mesh: %s", statusor.status().error_msg());
    return false;
  }

  std::unique_ptr<draco::Mesh> mesh = std::move(statusor).value();

  // Process attributes based on the GLTF mapping
  for (std::pair<std::string, int> attributeInfo : attributeTypeMap) {
    std::string semanticName = attributeInfo.first;
    int uniqueId = attributeInfo.second;

    const draco::PointAttribute *attribute =
        mesh->GetAttributeByUniqueId(uniqueId);
    if (attribute == nullptr) {
      pwarn("Draco mesh does not contain attribute with unique ID %d (semantic "
            "%s)",
            uniqueId, semanticName.c_str());
      continue;
    }

    VROGeometrySourceSemantic semantic = VROGeometrySourceSemantic::Invalid;
    if (semanticName == "POSITION") {
      semantic = VROGeometrySourceSemantic::Vertex;
    } else if (semanticName == "NORMAL") {
      semantic = VROGeometrySourceSemantic::Normal;
    } else if (semanticName == "TEXCOORD_0") {
      semantic = VROGeometrySourceSemantic::Texcoord;
    } else if (semanticName == "COLOR_0") {
      semantic = VROGeometrySourceSemantic::Color;
    } else if (semanticName == "JOINTS_0") {
      semantic = VROGeometrySourceSemantic::BoneIndices;
    } else if (semanticName == "WEIGHTS_0") {
      semantic = VROGeometrySourceSemantic::BoneWeights;
    }

    if (semantic != VROGeometrySourceSemantic::Invalid) {
      processAttribute(mesh.get(), attribute, semantic, sourcesOut);
    }
  }

  // Process Indices
  int numFaces = mesh->num_faces();
  int indicesCount = numFaces * 3;

  // Draco always decodes to triangles
  std::shared_ptr<VROByteBuffer> indexData =
      std::make_shared<VROByteBuffer>(indicesCount * sizeof(int));
  int *indices = (int *)indexData->getData();

  for (draco::FaceIndex i(0); i < numFaces; ++i) {
    const draco::Mesh::Face &face = mesh->face(i);
    indices[i.value() * 3 + 0] = face[0].value();
    indices[i.value() * 3 + 1] = face[1].value();
    indices[i.value() * 3 + 2] = face[2].value();
  }

  // Note: VROGeometryElement here implicitly applies to all accumulated sources
  // so far. This matches how VROGLTFLoader works (accumulating all sources for
  // a mesh).
  // Convert VROByteBuffer to VROData for VROGeometryElement
  indexData->releaseBytes(); // Transfer ownership to VROData
  std::shared_ptr<VROData> indexVROData = std::make_shared<VROData>(
      (void *)indexData->getData(), indicesCount * sizeof(int),
      VRODataOwnership::Move);

  std::shared_ptr<VROGeometryElement> element =
      std::make_shared<VROGeometryElement>(indexVROData,
                                           VROGeometryPrimitiveType::Triangle,
                                           indicesCount / 3, sizeof(int));
  elementsOut.push_back(element);

  return true;
}

void VRODracoMeshLoader::processAttribute(
    const draco::Mesh *mesh, const draco::PointAttribute *attribute,
    VROGeometrySourceSemantic semantic,
    std::vector<std::shared_ptr<VROGeometrySource>> &sources) {

  // For vertices, we need to create VROGeometrySources
  int componentCount = attribute->num_components();
  int vertexCount = (int)mesh->num_points();
  bool isFloat = attribute->data_type() == draco::DT_FLOAT32;

  // Allocate VRO buffer
  int bytesPerComponent =
      isFloat ? sizeof(float)
              : sizeof(float); // We convert everything to float for now
  // Support other types if needed, but VROGeometrySource primarily likes floats
  // especially for core attributes. Actually, Viro core can handle other types,
  // but let's see. The original VRODracoMeshLoader logic was just doing basic
  // float stuff.

  // Actually, let's keep it simple and just convert everything to float vectors
  // as Viro expects for now, or handle the types correctly. For BoneIndices, we
  // need Ints? VROGeometrySourceSemantic::BoneIndices expects standard types.
  // Let's stick to the previous implementation's data extraction but valid
  // semantics.

  // ... (Reusing existing body but removing semantic deduction)

  std::shared_ptr<VROData> vrod = std::make_shared<VROData>(
      (void *)nullptr, vertexCount * componentCount * bytesPerComponent);
  void *data = vrod->getData();

  for (int i = 0; i < vertexCount; ++i) {
    draco::AttributeValueIndex valIndex(i);
    // Move pointer to current vertex
    float *floatData =
        (float *)((char *)data + (i * componentCount * bytesPerComponent));

    // Read and convert to float
    if (!attribute->ConvertValue<float>(valIndex, componentCount, floatData)) {
      pwarn("Failed to convert Draco attribute value at index %d", i);
    }
  }

  std::shared_ptr<VROGeometrySource> source =
      std::make_shared<VROGeometrySource>(vrod, semantic, vertexCount, true,
                                          componentCount, bytesPerComponent, 0,
                                          componentCount * bytesPerComponent);
  sources.push_back(source);
}
