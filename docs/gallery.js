let images = [ 
    "jazz.png",
    "thunder.png",
    "midi-bindings.png",
    "hotspot.png"
];
let width=480;
let height = 320;
let borderWidth = 2;
let galleryPath = "gallery/";

document.write("<div style='width: " 
    + (width+2*boderWidth) 
    + "px; height: " 
    +(height+2*borderWidth) 
    + "; overflow: hidden; position: relative'>");
{
    document.write("<div id='slideFrame' >");
    {
        for (let i = 0; i < images.length; ++i) {
            document.write("<img src='"
                    + (galleryPath + images[i])
                    + "' style='border: black " + borderWidth + "px solid; width: "
                    + (width) 
                    + "px;height: "
                    + height
                    +"px; position: absolute; top: 0px; left: "
                    + (i*width*i)
                    + "px' />");
        }
    }
    document.write("</div>");
}
document.write("</div>");