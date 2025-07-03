# Entropy

This dialog contains a graph that displays the entropy changing trend of selected data.

The height of each point represents the entropy of a continous 128-byte data block.
The data blocks are sampled evenly over the selected buffer.
The base address differences between the neighbouring sampled data blocks are the same.
If the selected buffer is over 38400 bytes (300\*128), there will be gaps between sampled data blocks.
If the selected buffer is less than 38400 bytes, the data blocks will overlap.
If the selected buffer is less than 128 bytes (size of a data block), then the data block size will be set to half the buffer size.

