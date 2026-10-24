# Optional asset rebuild only; normal DreamSDK builds use committed title_art.c.
from pathlib import Path
from PIL import Image
root=Path(__file__).resolve().parent.parent
image=Image.open(root/'assets/title_art.png').convert('RGB').resize((512,512),Image.Resampling.LANCZOS)
data=image.tobytes()
pixels=[((data[i]>>3)<<11)|((data[i+1]>>2)<<5)|(data[i+2]>>3) for i in range(0,len(data),3)]
with (root/'src/title_art.c').open('w') as f:
    f.write('/* Generated RGB565 title texture. Rebuild: python3 tools/pack_title.py */\n#include "title_art.h"\nconst uint16_t title_pixels[512*512] __attribute__((aligned(32)))={\n')
    for i in range(0,len(pixels),16):f.write(','.join('0x%04x'%v for v in pixels[i:i+16])+',\n')
    f.write('};\n')
