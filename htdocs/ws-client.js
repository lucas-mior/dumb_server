var line1 = new TimeSeries();
var line2 = new TimeSeries();
setInterval(function() {
    line1.append(Date.now(), Math.random());
    line2.append(Date.now(), Math.random());
}, 1000);

var smoothie = new SmoothieChart({
    grid: { 
        strokeStyle: 'rgb(125, 0, 0)',
        fillStyle: 'rgb(60, 0, 0)',
        lineWidth: 1,
        millisPerLine: 250,
        verticalSections: 6 
    }
});
smoothie.addTimeSeries(
    line1,
    { 
        strokeStyle: 'rgb(0, 255, 0)',
        fillStyle: 'rgba(0, 255, 0, 0.4)',
        lineWidth: 3 
    }
);
smoothie.addTimeSeries(
    line2,
    { 
        strokeStyle: 'rgb(255, 0, 255)',
        fillStyle: 'rgba(255, 0, 255, 0.3)',
        lineWidth: 3 
    }
);

smoothie.streamTo(document.getElementById("mycanvas"), 1000);

function setUpCanvas() {
    canvas = document.getElementsByClassName("mycanvas")[0];
    ctx = canvas.getContext('2d');
    ctx.translate(0.5, 0.5);

    // Set display size (vw/vh).
    var sizeWidth = 80 * window.innerWidth / 100,
      sizeHeight = 80 * window.innerHeight / 100 || 766;

    //Setting the canvas site and width to be responsive 
    canvas.width = sizeWidth;
    canvas.height = sizeHeight;
    canvas.style.width = sizeWidth;
    canvas.style.height = sizeHeight;
}

window.addEventListener('onload', setUpCanvas, false);
window.addEventListener('resize', setUpCanvas, false);
