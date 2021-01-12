/*
originally from https://raw.githubusercontent.com/javl/image2cpp/master/index.html
converted to command line interface
node converter.js -f arduino -m horizontal1bit -b transparent -n VariableName -i input.bmp -o output.c
head output.c
// 'input', 100x100px
const unsigned char VariableName [] PROGMEM = {
	0x00, 0x00, 0x00

options:
-f : format can be one of plain, arduino, arduino_single, adafruit_gfx
-m : draw mode can be one of  horizontal1bit, vertical1bit, horizontal565, horizontalAlpha, horizontal888
-n : name of output variable
-i : input file .bmp file
-o : output file name e.g. output.c
-b : background color white, black, or transparent
-t : threshold for brightness / alpha threshold
-x : invert colors
*/
const FS = require('fs');
const { createCanvas, loadImage } = require('canvas')
const YArgs = require('yargs');

const argv = YArgs.option('format', {
  alias: 'f',
  description: 'output format',
  choices: ['plain', 'arduino', 'arduino_single', 'adafruit_gfx'],
  type: 'choices',
}).option('mode', {
  alias: 'm',
  description: 'draw mode effects the output byte ranges adjust as needed for your display',
  choices: ['horizontal1bit', 'vertical1bit', 'horizontal565', 'horizontalAlpha', 'horizontal888'],
  type: 'choices',
}).option('name', {
  alias: 'n',
  description: 'name of output variable',
  type: 'string',
}).option('input', {
  alias: 'i',
  description: 'input file .bmp file',
  type: 'string',
}).option('output', {
  alias: 'o',
  description: 'output file name e.g. output.c',
  type: 'string',
}).option('background', {
  alias: 'b',
  description: 'background color white, black, or transparent',
  type: 'string',
}).option('threshold', {
  alias: 't',
  description: 'threshold for brightness / alpha threshold',
  type: 'number'
}).option('invert', {
  alias: 'x',
  description: 'invert colors',
  type: 'boolean'
}).help().alias('help', 'h').argv;

class ConversionFunctions {
  // Output the image as a string for horizontally drawing displays
  static horizontal1bit(data, canvasWidth, canvasHeight) {
    let output_string = "";
    let output_index = 0;

    let byteIndex = 7;
    let number = 0;

    // format is RGBA, so move 4 steps per pixel
    for(let index = 0; index < data.length; index += 4){
      // Get the average of the RGB (we ignore A)
      const avg = (data[index] + data[index + 1] + data[index + 2]) / 3;
      if(avg > settings["threshold"]){
        number += Math.pow(2, byteIndex);
      }
      byteIndex--;

      // if this was the last pixel of a row or the last pixel of the
      // image, fill up the rest of our byte with zeros so it always contains 8 bits
      if ((index != 0 && (((index/4)+1)%(canvasWidth)) == 0 ) || (index == data.length-4)) {
        // for(var i=byteIndex;i>-1;i--){
          // number += Math.pow(2, i);
        // }
        byteIndex = -1;
      }

      // When we have the complete 8 bits, combine them into a hex value
      if(byteIndex < 0){
        var byteSet = number.toString(16);
        if(byteSet.length == 1){ byteSet = "0"+byteSet; }
        var b = "0x"+byteSet;
        output_string += b + ", ";
        output_index++;
        if(output_index >= 16){
          output_string += "\n";
          output_index = 0;
        }
        number = 0;
        byteIndex = 7;
      }
    }
    return output_string;
  }

  // Output the image as a string for vertically drawing displays
  static vertical1bit(data, canvasWidth, canvasHeight) {
    var output_string = "";
    var output_index = 0;

    for(var p=0; p < Math.ceil(settings["screenHeight"] / 8); p++){
      for(var x = 0; x < settings["screenWidth"]; x++){
        var byteIndex = 7;
        var number = 0;

        for (var y = 7; y >= 0; y--){
          var index = ((p*8)+y)*(settings["screenWidth"]*4)+x*4;
          var avg = (data[index] + data[index +1] + data[index +2]) / 3;
          if (avg > settings["threshold"]){
            number += Math.pow(2, byteIndex);
          }
          byteIndex--;
        }
        var byteSet = number.toString(16);
        if (byteSet.length == 1){ byteSet = "0"+byteSet; }
        var b = "0x"+byteSet.toString(16);
        output_string += b + ", ";
        output_index++;
        if(output_index >= 16){
          output_string += "\n";
          output_index = 0;
        }
      }
    }
    return output_string;
  }

  // Output the image as a string for 565 displays (horizontally)
  static horizontal565(data, canvasWidth, canvasHeight) {
    var output_string = "";
    var output_index = 0;

    // format is RGBA, so move 4 steps per pixel
    for(var index = 0; index < data.length; index += 4){
      // Get the RGB values
      var r = data[index];
      var g = data[index + 1];
      var b = data[index + 2];
      // calculate the 565 color value
      var rgb = ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | ((b & 0b11111000) >> 3);
      // Split up the color value in two bytes
      var firstByte = (rgb >> 8) & 0xff;
      var secondByte = rgb & 0xff;

      const byteSet = rgb.toString(16);
      while(byteSet.length < 4){ byteSet = "0" + byteSet; }
      output_string += "0x" + byteSet + ", ";

      // add newlines every 16 bytes
      output_index++;
      if(output_index >= 16){
        output_string += "\n";
        output_index = 0;
      }
    }
    return output_string;
  }
  // Output the image as a string for rgb888 displays (horizontally)
  static horizontal888(data, canvasWidth, canvasHeight) {
    var output_string = "";
    var output_index = 0;

    // format is RGBA, so move 4 steps per pixel
    for(var index = 0; index < data.length; index += 4){
      // Get the RGB values
      var r = data[index];
      var g = data[index + 1];
      var b = data[index + 2];
      // calculate the 565 color value
      var rgb = (r << 16) | (g << 8) | (b);
      // Split up the color value in two bytes
      var firstByte = (rgb >> 8) & 0xff;
      var secondByte = rgb & 0xff;

      const byteSet = rgb.toString(16);
      while(byteSet.length < 8){ byteSet = "0" + byteSet; }
      output_string += "0x" + byteSet + ", ";

      // add newlines every 16 bytes
      output_index++;
      if(output_index >= canvasWidth){
        output_string += "\n";
        output_index = 0;
      }
    }
    return output_string;
  }
  // Output the alpha mask as a string for horizontally drawing displays
  static horizontalAlpha(data, canvasWidth, canvasHeight) {
    var output_string = "";
    var output_index = 0;

    var byteIndex = 7;
    var number = 0;

    // format is RGBA, so move 4 steps per pixel
    for(var index = 0; index < data.length; index += 4){
      // Get alpha part of the image data
      var alpha = data[index + 3];
      if(alpha > settings["threshold"]){
        number += Math.pow(2, byteIndex);
      }
      byteIndex--;

      // if this was the last pixel of a row or the last pixel of the
      // image, fill up the rest of our byte with zeros so it always contains 8 bits
      if ((index != 0 && (((index/4)+1)%(canvasWidth)) == 0 ) || (index == data.length-4)) {
        byteIndex = -1;
      }

      // When we have the complete 8 bits, combine them into a hex value
      if(byteIndex < 0){
        var byteSet = number.toString(16);
        if(byteSet.length == 1){ byteSet = "0"+byteSet; }
        var b = "0x"+byteSet;
        output_string += b + ", ";
        output_index++;
        if(output_index >= 16){
          output_string += "\n";
          output_index = 0;
        }
        number = 0;
        byteIndex = 7;
      }
    }
    return output_string;
  }
}

// An images collection with helper methods
class Images {
  constructor() {
    this.collection = [];
  }

  push(img, canvas, glyph) {
    this.collection.push({ "img" : img, "canvas" : canvas, "glyph" : glyph });
  }

  remove(image) {
    let i = this.collection.indexOf(image);
    if(i != -1) this.collection.splice(i, 1);
  }
  each(f) { this.collection.forEach(f); }

  length() { return this.collection.length; }

  first() { return this.collection[0]; }
  last() { return this.collection[this.collection.length - 1]; }

  getByIndex(index) { return this.collection[index]; }
  setByIndex(index, img) { this.collection[index] = img; }
  get(img) {
    if (img) {
      for(let i = 0; i < this.collection.length; i++) {
        if(this.collection[i].img == img) {
          return this.collection[i];
        }
      }
    }
    return this.collection;
  }
}

// Filetypes accepted by the file picker
const fileTypes = ["jpg", "jpeg", "png", "bmp", "gif", "svg"];

// The variable to hold our images. Global so we can easily reuse it when the
// user updates the settings (change canvas size, scale, invert, etc)
const images = new Images();

// A bunch of settings used when converting
const settings = {
  screenWidth: 128,
  screenHeight: 64,
  scaleToFit: true,
  preserveRatio: true,
  centerHorizontally: false,
  centerVertically: false,
  backgroundColor: argv.background || "white",
  scale: "1",
  drawMode: argv.mode || "horizontal",
  threshold: argv.threshold || 128,
  outputFormat: argv.format || "plain",
  invertColors: false,
  conversionFunction: ConversionFunctions[argv.drawMode] || ConversionFunctions.horizontal1bit,
  identifier: argv.name || "myBitmap"
};

// Make the canvas black and white
function blackAndWhite(canvas, ctx){
  const imageData = ctx.getImageData(0,0,canvas.width, canvas.height);
  const data = imageData.data;
  for (let i = 0; i < data.length; i += 4) {
    let avg = (data[i] + data[i +1] + data[i +2]) / 3;
    avg > settings["threshold"] ? avg = 255 : avg = 0;
    data[i]     = avg; // red
    data[i + 1] = avg; // green
    data[i + 2] = avg; // blue
  }
  ctx.putImageData(imageData, 0, 0);
}


// Invert the colors of the canvas
function invert(canvas, ctx) {
  const imageData = ctx.getImageData(0,0,canvas.width, canvas.height);
  const data = imageData.data;
  for (let i = 0; i < data.length; i += 4) {
    data[i]     = 255 - data[i];     // red
    data[i + 1] = 255 - data[i + 1]; // green
    data[i + 2] = 255 - data[i + 2]; // blue
  }
  ctx.putImageData(imageData, 0, 0);
}

// Draw the image onto the canvas, taking into account color and scaling
function place_image(image) {
  const img = image.img;
  const canvas = image.canvas;
  const ctx = canvas.getContext("2d");
  image.ctx = ctx;

  // Make sure we're using the right canvas size
  //canvas.width = settings["screenWidth"];
  //canvas.height = settings["screenHeight"];

  // Invert background if needed
  if (settings["backgroundColor"] == "transparent") {
    ctx.fillStyle = "rgba(0,0,0,0.0)";
    ctx.globalCompositeOperation = 'copy';
  } else {
    if (settings["invertColors"]){
      settings["backgroundColor"] == "white" ? ctx.fillStyle = "black" : ctx.fillStyle = "white";
    } else {
      ctx.fillStyle = settings["backgroundColor"];
    }
    ctx.globalCompositeOperation = 'source-over';
  }
  ctx.fillRect(0, 0, canvas.width, canvas.height);

  // Offset used for centering the image when requested
  let offset_x = 0;
  let offset_y = 0;

  switch(settings["scale"]){
    case "1": // Original
      if(settings["centerHorizontally"]){ offset_x = Math.round((canvas.width - img.width) / 2); }
      if(settings["centerVertically"]){ offset_y = Math.round((canvas.height - img.height) / 2); }
      ctx.drawImage(img, 0, 0, img.width, img.height,
        offset_x, offset_y, img.width, img.height);
    break;
    case "2": // Fit (make as large as possible without changing ratio)
      var horRatio = canvas.width / img.width;
      var verRatio =  canvas.height / img.height;
      var useRatio  = Math.min(horRatio, verRatio);

      if(settings["centerHorizontally"]){ offset_x = Math.round((canvas.width - img.width*useRatio) / 2); }
      if(settings["centerVertically"]){ offset_y = Math.round((canvas.height - img.height*useRatio) / 2); }
      ctx.drawImage(img, 0, 0, img.width, img.height,
        offset_x, offset_y, img.width * useRatio, img.height * useRatio);
    break;
    case "3": // Stretch x+y (make as large as possible without keeping ratio)
      ctx.drawImage(img, 0, 0, img.width, img.height,
        offset_x, offset_y, canvas.width, canvas.height);
    break;
    case "4": // Stretch x (make as wide as possible)
      offset_x = 0;
      if(settings["centerVertically"]){ Math.round(offset_y = (canvas.height - img.height) / 2); }
      ctx.drawImage(img, 0, 0, img.width, img.height,
        offset_x, offset_y, canvas.width, img.height);
    break;
    case "5": // Stretch y (make as tall as possible)
      if(settings["centerHorizontally"]){ offset_x = Math.round((canvas.width - img.width) / 2); }
      offset_y = 0;
      ctx.drawImage(img, 0, 0, img.width, img.height,
        offset_x, offset_y, img.width, canvas.height);
    break;
  }
  // Make sure the image is black and white
  if(settings.conversionFunction == ConversionFunctions.horizontal1bit
    || settings.conversionFunction == ConversionFunctions.vertical1bit) {
    blackAndWhite(canvas, ctx);
    if(settings["invertColors"]){
      invert(canvas, ctx);
    }
  }
}


// Handle selecting an image with the file picker
async function handleImageSelection() {

  const img = await loadImage(argv.input);

  settings['screenWidth'] = img.width;
  settings['screenHeight'] = img.height;
  //console.log(`file resolution: ${img.width} x ${img.height}`);

  const canvas = createCanvas(img.width, img.height);
  images.push(img, canvas, argv.input.split(".")[0]);
  place_image(images.last());
}

function imageToString(image) {
  // extract raw image data
  const ctx = image.ctx;
  const canvas = image.canvas;

  const imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
  const data = imageData.data;
  return settings.conversionFunction(data, canvas.width, canvas.height);
}

// Get the custom arduino output variable name, if any
function getIdentifier() {
  return settings.identifier;
}


// Output the image string to the textfield
function outputString() {

  var output_string = "", count = 1;
  var code = "";

  switch(settings["outputFormat"]) {

    case "arduino": {

      images.each(function(image) {
        code = imageToString(image);

        // Trim whitespace from end and remove trailing comma
        code = code.replace(/,\s*$/,"");

        code = "\t" + code.split("\n").join("\n\t") + "\n";
        var variableCount = images.length() > 1 ? count++ : "";
        var comment = "// '" + image.glyph + "', "+image.canvas.width+"x"+image.canvas.height+"px\n";

        code = comment + "const " + getType() + " " +
            getIdentifier() +
            variableCount +
            " [] PROGMEM = {" +
            "\n" + code + "};\n";

        output_string += code;
      });
      break;
    }

    case "arduino_single": {
      var comment = "";
      images.each(function(image) {
        code = imageToString(image);
        code = "\t" + code.split("\n").join("\n\t") + "\n";
        comment = "\t// '" + image.glyph + ", " + image.canvas.width+"x"+image.canvas.height+"px\n";
        output_string += comment + code;
      });

      output_string = output_string.replace(/,\s*$/,"");

      output_string = "const " + getType() + " " +
        + getIdentifier()
        + " [] PROGMEM = {"
        + "\n" + output_string + "\n};";
      break;
    }

    case "adafruit_gfx": { // bitmap
      var comment = "";
      var useGlyphs = 0;
      images.each(function(image) {
        code = imageToString(image);
        code = "\t" + code.split("\n").join("\n\t") + "\n";
        comment = "\t// '" + image.glyph + ", " + image.canvas.width+"x"+image.canvas.height+"px\n";
        output_string += comment + code;
        if(image.glyph.length == 1) useGlyphs++;
      });

      output_string = output_string.replace(/,\s*$/,"");
      output_string = "const unsigned char "
        + getIdentifier()
        + "Bitmap"
        + " [] PROGMEM = {"
        + "\n" + output_string + "\n};\n\n"
        + "const GFXbitmapGlyph "
        + getIdentifier()
        + "Glyphs [] PROGMEM = {\n";

      var firstAschiiChar = document.getElementById("first-ascii-char").value;
      var xAdvance = parseInt(document.getElementById("x-advance").value);
      var offset = 0;
      code = "";

      // GFXbitmapGlyph
      images.each(function(image) {
        code += "\t{ "
          + offset + ", "
          + image.canvas.width + ", "
          + image.canvas.height + ", "
          + xAdvance + ", "
          + "'" + (images.length() == useGlyphs ?
            image.glyph :
              String.fromCharCode(firstAschiiChar++)) + "'"
          + " }";
        if(image != images.last()){ code += ","; }
        code += "// '" + image.glyph + "'\n";
        offset += image.canvas.width;
      });
      code += "};\n";
      output_string += code;

      // GFXbitmapFont
      output_string += "\nconst GFXbitmapFont "
        + getIdentifier()
        + "Font PROGMEM = {\n"
        + "\t(uint8_t *)"
        + getIdentifier() + "Bitmap,\n"
        + "\t(GFXbitmapGlyph *)"
        + getIdentifier()
        + "Glyphs,\n"
        + "\t" + images.length()
        + "\n};\n";
      break;
    }
    default: { // plain
      images.each(function(image) {
        code = imageToString(image);
        var comment = image.glyph ? ("// '" + image.glyph + "', " + image.canvas.width+"x"+image.canvas.height+"px\n") : "";
        if(image.img != images.first().img) comment = "\n" + comment;
        code = comment + code;
        output_string += code;
      });
      // Trim whitespace from end and remove trailing comma
      output_string = output_string.replace(/,\s*$/g,"");
    }
  }

  return output_string;
}

// Use the horizontally oriented list to draw the image
function listToImageHorizontal(list, canvas){

  var ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  var imgData = ctx.createImageData(canvas.width, canvas.height);

  var index = 0;

  var page = 0;
  var x = 0;
  var y = 7;
  // round the width up to the next byte
  var widthRoundedUp = Math.floor(canvas.width / 8 + (canvas.width % 8 ? 1 : 0)) * 8;
  var widthCounter = 0;

  // Move the list into the imageData object
  for (var i=0;i<list.length;i++){

    var binString = hexToBinary(list[i]);
    if(!binString.valid){
      alert("Something went wrong converting the string. Did you forget to remove any comments from the input?");
      console.log("invalid hexToBinary: ", binString.s);
      return;
    }
    binString = binString.result;
    if (binString.length == 4){
      binString = binString + "0000";
    }

    // Check if pixel is white or black
    for(var k=0; k<binString.length; k++, widthCounter++){
      // if we've counted enough bits, reset counter for next line
      if(widthCounter >= widthRoundedUp) {
        widthCounter = 0;
      }
      // skip 'artifact' pixels due to rounding up to a byte
      if(widthCounter >= canvas.width) {
        continue;
      }
      var color = 0;
      if(binString.charAt(k) == "1"){
        color = 255;
      }
      imgData.data[index] = color;
      imgData.data[index+1] = color;
      imgData.data[index+2] = color;
      imgData.data[index+3] = 255;

      index += 4;
    }
  }

  // Draw the image onto the canvas, then save the canvas contents
  // inside the img object. This way we can reuse the img object when
  // we want to scale / invert, etc.
  ctx.putImageData(imgData, 0, 0);
  var img = new Image();
  img.src = canvas.toDataURL("image/png");
  images.first().img = img;
}


// Use the vertically oriented list to draw the image
function listToImageVertical(list, canvas){

  var ctx = canvas.getContext("2d");
  ctx.clearRect(0, 0, canvas.width, canvas.height);

  var index = 0;

  var page = 0;
  var x = 0;
  var y = 7;

  // Move the list into the imageData object
  for (var i=0;i<list.length;i++){

    var binString = hexToBinary(list[i]);
    if(!binString.valid){
      alert("Something went wrong converting the string. Did you forget to remove any comments from the input?");
      console.log("invalid hexToBinary: ", binString.s);
      return;
    }
    binString = binString.result;
    if (binString.length == 4){
      binString = binString + "0000";
    }

    // Check if pixel is white or black
    for(var k=0; k<binString.length; k++){
      var color = 0;
      if(binString.charAt(k) == "1"){
        color = 255;
      }
      drawPixel(ctx, x, (page*8)+y, color);
      y--;
      if(y < 0){
        y = 7;
        x++;
        if(x >= settings["screenWidth"]){
          x = 0;
          page++;
        }
       }

    }
  }
  // Save the canvas contents inside the img object. This way we can
  // reuse the img object when we want to scale / invert, etc.
  var img = new Image();
  img.src = canvas.toDataURL("image/png");
  images.first().img = img;
}


// Convert hex to binary
function hexToBinary(s) {

  var i, k, part, ret = "";
  // lookup table for easier conversion. "0" characters are
  // padded for "1" to "7"
  var lookupTable = {
    "0": "0000", "1": "0001", "2": "0010", "3": "0011", "4": "0100",
    "5": "0101", "6": "0110", "7": "0111", "8": "1000", "9": "1001",
    "a": "1010", "b": "1011", "c": "1100", "d": "1101", "e": "1110",
    "f": "1111", "A": "1010", "B": "1011", "C": "1100", "D": "1101",
    "E": "1110", "F": "1111"
  };
  for (i = 0; i < s.length; i += 1) {
    if (lookupTable.hasOwnProperty(s[i])) {
      ret += lookupTable[s[i]];
    } else {
      return { valid: false, s: s };
    }
  }
  return { valid: true, result: ret };
}


// Quick and effective way to draw single pixels onto the canvas
// using a global 1x1px large canvas
function drawPixel(ctx, x, y, color) {
  var single_pixel = ctx.createImageData(1,1);
  var d = single_pixel.data;

  d[0] = color;
  d[1] = color;
  d[2] = color;
  d[3] = 255;
  ctx.putImageData(single_pixel, x, y);
}
// get the type (in arduino code) of the output image
// this is a bit of a hack, it's better to make this a property of the conversion function (should probably turn it into objects)
function getType() {
  if (settings.conversionFunction == ConversionFunctions.horizontal565) {
    return "uint16_t";
  }else if(settings.conversionFunction == ConversionFunctions.horizontal888){
    return "unsigned long";
  }else{
    return "unsigned char";
  }
}

try {
  (async function() {
    console.log(argv);
    await handleImageSelection();
    const output = outputString();
    console.log(`writing: ${argv.output}`);
    FS.writeFile(argv.output, output, "binary", function() {
    });
  })();
} catch(e) {
  console.error(e);
}
