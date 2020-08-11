/**
 * @fileOverview A simple one dimensional histogram.
 * @author <a href="mailto:thomas.bretz@epfl.ch">Thomas Bretz</a>
 */
'use strict';

/**
 *
 * @constructor
 *
 * @param {Interger} nx
 *
 * @param {Number} xmin
 *
 * @param {Number} xmax
 *
 * @returns
 *     A sub-classed array with the Hist1D functions added as properties.
 *
 * @example
 *     var hist = Hist1D(10, -0.5, 1.5);
 *
 */
function Hist1D(nx, xmin, xmax)
{
    /**
     *
     * Array
     *
     */
    var arr = new Array(nx);

    /**
     *
     * @exports arr.get as Hist1D.get
     *
     */
    arr.get = function(x)
    {
        var ix = parseInt(nx*(x-xmin)/(xmax-xmin));

        return arr[ix] ? arr[ix] : 0;
    }

    /**
     *
     * @exports arr.fill as Hist1D.fill
     *
     */
    arr.fill = function(x, w)
    {
        if (!x || x===NaN)
            return false;

        var ix = parseInt(nx*(x-xmin)/(xmax-xmin));
        if (ix<0 || ix>=nx)
            return false;

        if (!arr[ix])
            arr[ix] = 0;

        arr[ix] += w ? w : 1;

        return true;
    }

    /**
     *
     * @exports arr.print as Hist1D.print
     *
     */
    arr.print = function(len)
    {
        if (!len)
            len = 40;
        if (len<6)
            len = 6;

        var sum = arr.reduce(function(a,b){return a+b;}, 0);
        var max = arr.reduce(function(a,b){return Math.max(a,b);}, 0);

        console.out("");
        for (var ix=nx-1; ix>=0; ix--)
        {
            var entry = arr[ix] ? arr[ix] : 0;

            var line ="%3d [%3d%] ".$(ix, sum==0 ? 0 : 100*entry/sum);
            if (arr[ix])
            {
                var val = parseInt(len*arr[ix]/max);
                var entry = ("%"+val+"s").$("");
                entry = entry.replace(/ /g, "*");
                line += ("%-"+len+"s").$(entry)+" |";
            }

            console.out(line);
        }

        var entry = arr[ix] ? arr[ix] : 0;
        console.out("   --------"+("%"+(len-5)+"s").$("")+"-------");

        var line =" %9d  ".$(sum);
        line += ("%"+len+"d").$(max);
        console.out(line);
        console.out("");
    }

    return arr;
}
