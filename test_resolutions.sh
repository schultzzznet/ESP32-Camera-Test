#!/bin/bash

# ESP32-CAM Resolution Test Script
# Tests all supported resolutions and saves to jpgs/ directory

IP="192.168.1.29"
DIR="jpgs"

echo "================================================"
echo "ESP32-CAM Dual-Mode Resolution Test"
echo "================================================"
echo "Target: http://$IP"
echo "Output: $DIR/"
echo ""

# Create output directory
mkdir -p "$DIR"

echo "Testing RGB565 Mode (≤SVGA - Software JPEG):"
echo "-------------------------------------------"

# Test QVGA - 320x240
echo -n "QVGA (320x240)... "
curl -s "http://$IP/capture?res=qvga" -o "$DIR/qvga_320x240.jpg"
if [ -f "$DIR/qvga_320x240.jpg" ]; then
    SIZE=$(wc -c < "$DIR/qvga_320x240.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/qvga_320x240.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

# Test VGA - 640x480
echo -n "VGA (640x480)... "
curl -s "http://$IP/capture?res=vga" -o "$DIR/vga_640x480.jpg"
if [ -f "$DIR/vga_640x480.jpg" ]; then
    SIZE=$(wc -c < "$DIR/vga_640x480.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/vga_640x480.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

# Test SVGA - 800x600
echo -n "SVGA (800x600)... "
curl -s "http://$IP/capture?res=svga" -o "$DIR/svga_800x600.jpg"
if [ -f "$DIR/svga_800x600.jpg" ]; then
    SIZE=$(wc -c < "$DIR/svga_800x600.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/svga_800x600.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

echo ""
echo "Testing Hardware JPEG Mode (XGA+ - Direct JPEG):"
echo "------------------------------------------------"

# Test XGA - 1024x768
echo -n "XGA (1024x768)... "
curl --max-time 60 -s "http://$IP/capture?res=xga" -o "$DIR/xga_1024x768.jpg"
if [ -f "$DIR/xga_1024x768.jpg" ]; then
    SIZE=$(wc -c < "$DIR/xga_1024x768.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/xga_1024x768.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

# Test HD - 1280x720
echo -n "HD (1280x720)... "
curl --max-time 90 -s "http://$IP/capture?res=hd" -o "$DIR/hd_1280x720.jpg"
if [ -f "$DIR/hd_1280x720.jpg" ]; then
    SIZE=$(wc -c < "$DIR/hd_1280x720.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/hd_1280x720.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

# Test SXGA - 1280x1024
echo -n "SXGA (1280x1024)... "
curl --max-time 90 -s "http://$IP/capture?res=sxga" -o "$DIR/sxga_1280x1024.jpg"
if [ -f "$DIR/sxga_1280x1024.jpg" ]; then
    SIZE=$(wc -c < "$DIR/sxga_1280x1024.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/sxga_1280x1024.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

# Test UXGA - 1600x1200 (Maximum)
echo -n "UXGA (1600x1200)... "
curl --max-time 120 -s "http://$IP/capture?res=uxga" -o "$DIR/uxga_1600x1200.jpg"
if [ -f "$DIR/uxga_1600x1200.jpg" ]; then
    SIZE=$(wc -c < "$DIR/uxga_1600x1200.jpg" | tr -d ' ')
    echo "✓ ${SIZE} bytes"
    jpeginfo "$DIR/uxga_1600x1200.jpg" 2>/dev/null | grep -E "^$DIR" | awk '{print "  "$2" x "$4}'
else
    echo "✗ Failed"
fi

echo ""
echo "================================================"
echo "Summary:"
echo "================================================"
ls -lh "$DIR"/*.jpg 2>/dev/null | awk '{print $9, "-", $5}'
echo ""
echo "Validating all JPEGs:"
jpeginfo "$DIR"/*.jpg 2>/dev/null | grep -E "(OK|WARNING|ERROR)"
echo ""
echo "Done! Images saved to $DIR/"
