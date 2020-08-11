'use strict';

function Hist2D(nx, xmin, xmax, ny, ymin, ymax)
{
    var arr = new Array(nx);

    arr.get = function(x, y)
    {
        var ix = parseInt(nx*(x-xmin)/(xmax-xmin));
        var iy = parseInt(ny*(y-ymin)/(ymax-ymin));

        if (!arr[ix])
            return 0;

        if (!arr[ix][iy])
            return 0;

        return arr[ix][iy];
    }

    arr.fill = function(x, y, w)
    {
        var ix = parseInt(nx*(x-xmin)/(xmax-xmin));
        var iy = parseInt(ny*(y-ymin)/(ymax-ymin));

        if (ix<0 || ix>=nx || iy<0 || iy>=ny)
            return false;

        if (!arr[ix])
            arr[ix] = new Array(ny);

        if (!arr[ix][iy])
            arr[ix][iy] = 0;

        arr[ix][iy] += w ? w : 1;

        return true;
    }

    arr.print = function()
    {
        var line1 = "   |";
        for (var ix=0; ix<nx; ix++)
            line1 += " %3d ".$(ix);

        var line2 = "---+";
        for (var ix=0; ix<nx; ix++)
            line2 += "+----";
        line2+="+----";

        console.out("", line1, line2);

        var sum = 0;
        var sx = [];
        for (var ix=0; ix<nx; ix++)
            sx[ix] = 0;

        for (var iy=ny-1; iy>=0; iy--)
        {
            var line = "%3d|".$(iy);

            var sy = 0;
            for (var ix=0; ix<nx; ix++)
            {
                var val = arr[ix] ? arr[ix][iy] : "";
                line += " %4s".$(val?val:"");

                if (arr[ix])
                {
                    sy     += val?val:0;
                    sx[ix] += val?val:0;
                }
            }

            sum += sy;

            console.out(line+"|%4d".$(sy));
        }

        console.out(line2);

        line = "   |";
        for (var ix=0; ix<nx; ix++)
            line += " %4d".$(sx[ix]);

        console.out(line+"|%4d".$(sum), "");
    }

    return arr;
}
