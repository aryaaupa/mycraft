from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
p=Path(__file__).resolve().parents[1]
canv=Image.new('RGB',(1650,510),'#f6f0e8');d=ImageDraw.Draw(canv)
for idx,name in enumerate(['menu','instructions','game','cottage','ending']):
 im=Image.open(p/(name+'.ppm'));draw=ImageDraw.Draw(im);font=ImageFont.load_default(size=8)
 for line in (p/(name+'_text.tsv')).read_text().splitlines():
  x,y,c,txt=line.split('\t',3);c=int(c);color=(((c>>11)&31)*255//31,((c>>5)&63)*255//63,(c&31)*255//31)
  for j,ch in enumerate(txt):draw.text((int(x)+j*6,int(y)),ch,font=font,fill=color)
 canv.paste(im.resize((256,320),Image.Resampling.NEAREST),(55+idx*325,90))
 d.text((55+idx*325,55),name.upper(),font=ImageFont.load_default(size=20),fill='#41454f')
d.text((55,440),'ACTUAL PIXEL FONT - 2x menu labels and score; verify colors on the LCD.',font=ImageFont.load_default(size=17),fill='#41454f')
canv.save(p/'MyCraft_Preview.png')
