
#!/bin/bash

COMBINED_DIR=$(dirname $(realpath $0))

echo "Flashing combined image..."
st-flash --reset --format ihex write $COMBINED_DIR/full_image.hex
echo "✅ Done!"