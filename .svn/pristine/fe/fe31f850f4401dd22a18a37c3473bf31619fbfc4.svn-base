'use strict';

function Handler(name)
{
    this.name  = name;
    this.array = [];

    this.add = function(func)
    {
        this.array.push(func);
        //console.out(this.name+":add [N="+this.array.length+"]");
    }

    this.run = function(timeout)
    {
        console.out(this.name+":start");

        var rc = [];

        var start = new Date();
        while (!timeout || (new Date()-start)<timeout)
        {
            var done = true;

            //rc = rc.map(this.array[i]);
            //
            //rc.forEach(function(el){ done &= el && el.length==0;);

            for (var i=0; i<this.array.length; i++)
            {
                rc[i] = this.array[i](rc[i]);
                if (rc[i]===undefined || rc[i].length>0)
                    done = false;
            }

            if (done)
            {
                console.out(this.name+":success [time="+(new Date()-start)+"ms]");
                return true;
            }

            v8.sleep();
        }

        console.out(this.name+":timeout ["+timeout+"ms]");
        return false;
    }
}
