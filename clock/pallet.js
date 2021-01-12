/*
create the color pallet image for epaper 5.6, 7 colors (module f)
see: https://www.waveshare.com/wiki/5.65inch_e-Paper_Module_(F)
*/
const { createCanvas } = require('canvas')
const fs = require('fs');

const colors = [//[0, 0, 0, 255], // black 0b0000
                [255,255,255,255], // white 0b0001
                [0,255,0,255], // green 0b0010
                [0,0,255,255], // blue 0b0011 
                [255,0,0,255], // red 0b0100
                [255,255,0,255], // yellow 0b0101
                [255,165,0,255]]; // orange 0b0110

const canvas = createCanvas(98, 98);
const context = canvas.getContext('2d');
const imageData = context.getImageData(0, 0, 98, 98);

/*
  create stripes of color 14 squares tall  e.g. (98 / 7 = 14 or (98*4) / 7 = 56 points
  98x98x4 = 38416 rgba pixels
*/
let colorIndex = 0; // 7 colors
let row = 0;
const data = imageData.data;
for (let i = 0, len = data.length; i < len; i+=4) { // rgba
  const color = colors[colorIndex];
  //if (!color) { console.error(`${colorIndex} not found in :`, colors); break; }

  data[i] = color[0]; // red
  data[i+1] = color[1]; // green
  data[i+2] = color[2]; // blue
  data[i+3] = color[3]; // alpha

  if (i % 392 == 0) { // we've traveled at least 1 row of pixels (98 * 4 = 392)
    row++;
    if (row == 14) { // 14 rows, switch at every 15th
      if (colorIndex < colors.length - 1) {
        colorIndex++;
      }
      row = 0;
      //console.log(i, row, colorIndex, len);
    }
  }
}

context.putImageData(imageData, 0, 0);
const buffer = canvas.toBuffer('image/png')
fs.writeFileSync('./eink-7color.png', buffer)
