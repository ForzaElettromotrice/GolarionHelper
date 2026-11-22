.pragma library

const heightToWidthRatio = 1.2;

function trace(ctx, width, height) {
    ctx.beginPath();
    ctx.moveTo(width / 2, 4);
    ctx.lineTo(width - 4, height * 0.17);
    ctx.lineTo(width - 4, height * 0.46);
    ctx.bezierCurveTo(width - 4, height * 0.69, width * 0.79, height * 0.86, width / 2, height - 4);
    ctx.bezierCurveTo(width * 0.21, height * 0.86, 4, height * 0.69, 4, height * 0.46);
    ctx.lineTo(4, height * 0.17);
    ctx.closePath();
}
