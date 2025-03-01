


 * https://freetype.org/freetype2/docs/tutorial/step1.html#section-7

```c
void create_text() {
  if (vkCtx.text.exists) {
      printf("Text already exists, skipping creation\n");
      return;
  }

  if (!ft_library) {
      if (FT_Init_FreeType(&ft_library) != 0) {
          fprintf(stderr, "Failed to initialize FreeType\n");
          exit(1);
      }
  }

  FT_Face face;
  if (FT_New_Face(ft_library, "FiraSans-Bold.ttf", 0, &face) != 0) {
      fprintf(stderr, "Failed to load font FiraSans-Bold.ttf - check if file exists in executable directory\n");
      exit(1);
  }
  FT_Set_Pixel_Sizes(face, 0, 48);

  const char* text = "Hello World";
  int atlas_width = 0, atlas_height = 0;
  for (const char* c = text; *c; c++) {
      if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
      atlas_width += face->glyph->bitmap.width + 2;
      atlas_height = (face->glyph->bitmap.rows > atlas_height) ? face->glyph->bitmap.rows : atlas_height;
  }

  unsigned char* atlas_data = calloc(1, atlas_width * atlas_height);
  int x_offset = 0;
  for (const char* c = text; *c; c++) {
      if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
      FT_Bitmap* bitmap = &face->glyph->bitmap;
      for (unsigned int y = 0; y < bitmap->rows; y++) {
          for (unsigned int x = 0; x < bitmap->width; x++) {
              atlas_data[(y * atlas_width) + x_offset + x] = bitmap->buffer[y * bitmap->width + x];
          }
      }
      x_offset += bitmap->width + 2;
  }

  printf("Atlas dimensions: %d x %d\n", atlas_width, atlas_height);
  for (int y = 0; y < atlas_height; y++) {
      for (int x = 0; x < atlas_width; x++) {
          printf("%c", atlas_data[y * atlas_width + x] > 128 ? '#' : ' ');
      }
      printf("\n");
  }

  VkBuffer stagingBuffer;
  VmaAllocation stagingAlloc;
  VkBufferCreateInfo stagingBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  stagingBufferInfo.size = atlas_width * atlas_height;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo stagingAllocInfo = {};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  if (vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocInfo, &stagingBuffer, &stagingAlloc, NULL) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create staging buffer for text texture\n");
      exit(1);
  }

  void* data;
  vmaMapMemory(allocator, stagingAlloc, &data);
  memcpy(data, atlas_data, atlas_width * atlas_height);
  vmaUnmapMemory(allocator, stagingAlloc);

  VkImageCreateInfo textImageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  textImageInfo.imageType = VK_IMAGE_TYPE_2D;
  textImageInfo.format = VK_FORMAT_R8_UNORM;
  textImageInfo.extent.width = atlas_width;
  textImageInfo.extent.height = atlas_height;
  textImageInfo.extent.depth = 1;
  textImageInfo.mipLevels = 1;
  textImageInfo.arrayLayers = 1;
  textImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  textImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  textImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  textImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  textImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VmaAllocationCreateInfo texAllocInfo = {};
  texAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  if (vmaCreateImage(allocator, &textImageInfo, &texAllocInfo, &vkCtx.text.texture, &vkCtx.text.texAlloc, NULL) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create text texture with VMA\n");
      exit(1);
  }

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo cmdAllocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAllocInfo.commandPool = vkCtx.commandPool;
  cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAllocInfo.commandBufferCount = 1;
  vkAllocateCommandBuffers(vkCtx.device, &cmdAllocInfo, &commandBuffer);

  VkCommandBufferBeginInfo cmdBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo);

  VkImageMemoryBarrier barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = vkCtx.text.texture;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  VkBufferImageCopy copyRegion = {};
  copyRegion.bufferOffset = 0;
  copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  copyRegion.imageSubresource.mipLevel = 0;
  copyRegion.imageSubresource.baseArrayLayer = 0;
  copyRegion.imageSubresource.layerCount = 1;
  copyRegion.imageExtent.width = atlas_width;
  copyRegion.imageExtent.height = atlas_height;
  copyRegion.imageExtent.depth = 1;

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkCtx.text.texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;
  vkQueueSubmit(vkCtx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(vkCtx.graphicsQueue);

  vkFreeCommandBuffers(vkCtx.device, vkCtx.commandPool, 1, &commandBuffer);
  vmaDestroyBuffer(allocator, stagingBuffer, stagingAlloc);

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = vkCtx.text.texture;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(vkCtx.device, &viewInfo, NULL, &vkCtx.text.textureView) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create text texture view\n");
      exit(1);
  }

  VkDescriptorImageInfo textDescriptorImageInfo = {};
  textDescriptorImageInfo.sampler = vkCtx.textureSampler;
  textDescriptorImageInfo.imageView = vkCtx.text.textureView;
  textDescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkWriteDescriptorSet descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = vkCtx.descriptorSet;
  descriptorWrite.dstBinding = 1;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &textDescriptorImageInfo;

  vkUpdateDescriptorSets(vkCtx.device, 1, &descriptorWrite, 0, NULL);

  float vertices[] = {
      -0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  1.0f, // Bottom-left (texture bottom-left)
      -0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (texture top-left)
       0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f, // Bottom-right (texture bottom-right)
      -0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (texture top-left)
       0.5f,  0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  1.0f, // Top-right (texture top-right)
       0.5f, -0.1f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f  // Bottom-right (texture bottom-right)
  };

  VkBufferCreateInfo textBufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  textBufferInfo.size = sizeof(vertices);
  textBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  textBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo textAllocInfo = {};
  textAllocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  if (vmaCreateBuffer(allocator, &textBufferInfo, &textAllocInfo, &vkCtx.text.buffer, &vkCtx.text.allocation, NULL) != VK_SUCCESS) {
      fprintf(stderr, "Failed to create text buffer with VMA\n");
      exit(1);
  }

  vmaMapMemory(allocator, vkCtx.text.allocation, &data);
  memcpy(data, vertices, sizeof(vertices));
  vmaUnmapMemory(allocator, vkCtx.text.allocation);

  vkCtx.text.vertexCount = 6;
  vkCtx.text.exists = true;
  printf("Text 'Hello World' created with VMA\n");

  free(atlas_data);
  FT_Done_Face(face);
}
```


# FreeType Glyph Metrics (Section 7 Reference)

The FreeType tutorial explains key metrics that affect glyph positioning:
 * bitmap_left: Horizontal distance from the origin (left edge) to the glyph’s leftmost pixel.
 * bitmap_top: Vertical distance from the baseline to the topmost pixel (positive upward).
 * advance.x: Horizontal distance to move the pen position after drawing the glyph.
 * advance.y: Vertical advance (typically 0 for horizontal text).

 * Baseline: An imaginary line where the glyphs "sit" (e.g., "H" sits on it, "g" hangs below).

In our current code:
 * We use face->glyph->bitmap.width and bitmap.rows for size.
 * We align all glyphs to the atlas’s top (y=0), ignoring bitmap_top or baseline offsets.
 * This top-alignment carries over to the plane, causing "Hello World" to anchor at the top.

Correct Format?
Yes, the FreeType format you referenced is correct for rendering individual glyphs into a bitmap (like our atlas). However, we’re missing:
 * Baseline Alignment: Adjusting each glyph’s vertical position based on bitmap_top to align to a common baseline.
 * Bottom Alignment: Shifting the plane’s y coordinates to place the baseline near the screen bottom, not the atlas top.

# Current create_text() Recap
 * Atlas: 
   * 248 x 38 pixels.
   * Glyphs are top-aligned at y=0 (e.g., "H" top at y=0, bottom at y=32).

 * Plane: 
   * y=-0.1 (bottom, v=1, atlas bottom) to y=0.1 (top, v=0, atlas top).
   * Projection flips y, so y=0.1 is screen bottom, y=-0.1 is screen top.

 * Result: "H" (atlas top, v=0) is at y=0.1 (screen bottom), extending upward, anchoring to the plane’s top.

# Missing Config: Baseline and Bottom Alignment

To fix this, we need:

 * Baseline in Atlas: Adjust glyph positions using face->glyph->bitmap_top so they align to a baseline, not the atlas top.
 * Plane Adjustment: Map the baseline to the plane’s bottom y, ensuring it’s near the screen bottom.

## Step 1: Adjust Atlas for Baseline
Modify create_text() to offset glyphs vertically based on bitmap_top:
```c
void create_text() {
    if (vkCtx.text.exists) {
        printf("Text already exists, skipping creation\n");
        return;
    }

    if (!ft_library) {
        if (FT_Init_FreeType(&ft_library) != 0) {
            fprintf(stderr, "Failed to initialize FreeType\n");
            exit(1);
        }
    }

    FT_Face face;
    if (FT_New_Face(ft_library, "FiraSans-Bold.ttf", 0, &face) != 0) {
        fprintf(stderr, "Failed to load font FiraSans-Bold.ttf - check if file exists\n");
        exit(1);
    }
    FT_Set_Pixel_Sizes(face, 0, 48);

    const char* text = "Hello World";
    int atlas_width = 0, atlas_height = 0;
    int max_bearing_y = 0;

    // First pass: Calculate dimensions and max bearing
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        atlas_width += face->glyph->bitmap.width + 2;
        atlas_height = (face->glyph->bitmap.rows > atlas_height) ? face->glyph->bitmap.rows : atlas_height;
        max_bearing_y = (face->glyph->bitmap_top > max_bearing_y) ? face->glyph->bitmap_top : max_bearing_y;
    }

    // Adjust atlas height to include space below baseline
    int baseline_y = max_bearing_y; // Baseline at max bearing from top
    atlas_height += max_bearing_y; // Add space for descenders (e.g., "d")

    unsigned char* atlas_data = calloc(1, atlas_width * atlas_height);
    int x_offset = 0;

    // Second pass: Render glyphs with baseline offset
    for (const char* c = text; *c; c++) {
        if (FT_Load_Char(face, *c, FT_LOAD_RENDER) != 0) continue;
        FT_Bitmap* bitmap = &face->glyph->bitmap;
        int y_offset = baseline_y - face->glyph->bitmap_top; // Offset from baseline
        for (unsigned int y = 0; y < bitmap->rows; y++) {
            for (unsigned int x = 0; x < bitmap->width; x++) {
                int atlas_y = y_offset + y;
                if (atlas_y >= 0 && atlas_y < atlas_height) {
                    atlas_data[(atlas_y * atlas_width) + x_offset + x] = bitmap->buffer[y * bitmap->width + x];
                }
            }
        }
        x_offset += bitmap->width + 2;
    }

    printf("Atlas dimensions: %d x %d, Baseline at y=%d\n", atlas_width, atlas_height, baseline_y);
    for (int y = 0; y < atlas_height; y++) {
        for (int x = 0; x < atlas_width; x++) {
            printf("%c", atlas_data[y * atlas_width + x] > 128 ? '#' : ' ');
        }
        printf("\n");
    }

    // ... (Vulkan texture creation remains the same until vertex data)

    float vertices[] = {
        -0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 1.0f,  1.0f, // Bottom-left (u=0, v=1)
        -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (u=0, v=0)
         0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f, // Bottom-right (u=1, v=1)
        -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (u=0, v=0)
         0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  1.0f, // Top-right (u=1, v=0)
         0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 1.0f,  1.0f  // Bottom-right (u=1, v=1)
    };

    // ... (Rest of Vulkan buffer creation remains unchanged)
}
```

# Key Changes and Explanation

 * Baseline Calculation:
   * max_bearing_y: Finds the tallest glyph’s bitmap_top (e.g., "H" ≈ 33px).
   * baseline_y = max_bearing_y: Sets the baseline at this height from the atlas top.
   * atlas_height += max_bearing_y: Increases atlas height to fit descenders (e.g., "d" below baseline).

 * Glyph Positioning:
 *  * y_offset = baseline_y - face->glyph->bitmap_top: Offsets each glyph so its top aligns relative to the baseline.

 * Example:
   * "H": bitmap_top ≈ 33, y_offset = 33 - 33 = 0, top at y=0.
   * "e": bitmap_top ≈ 30, y_offset = 33 - 30 = 3, top at y=3.

 * Ensures all glyphs sit on the baseline, not the atlas top.

 * Atlas Layout:
   * New atlas_height ≈ 71 (38 + 33), with baseline at y=33.
   * "H" top at y=0, bottom at y=32; "d" top at y=1, bottom at y=38.

# Testing This
 * Expected Atlas: Logs will show the baseline at y ≈ 33, with "Hello World" glyphs aligned to it, not y=0.
 * Plane: Still y=-0.2 to 0.2, so we’ll adjust UVs next to map the baseline correctly.

## Step 2: Adjust Plane for Bottom Alignment
Once the atlas uses a baseline, update the vertices to map the baseline to the plane’s bottom:

```c
float vertices[] = {
    -0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, (float)baseline_y / atlas_height,  1.0f, // Bottom-left (baseline)
    -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left (atlas top)
     0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, (float)baseline_y / atlas_height,  1.0f, // Bottom-right
    -0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  0.0f, 0.0f,  1.0f, // Top-left
     0.5f,  0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, 0.0f,  1.0f, // Top-right
     0.5f, -0.2f, 0.0f,  1.0f, 1.0f, 1.0f,  1.0f, (float)baseline_y / atlas_height,  1.0f  // Bottom-right
};
```
 * UVs: v = baseline_y / atlas_height (≈ 33/71 ≈ 0.465) maps the baseline to y=-0.2 (screen bottom).








