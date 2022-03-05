var ToobGain = {
    CreateWrapper: function (selector,width,height)
    {
        var result = { 
            MIN_DB_AF: Math.pow(10,-192/20),
            width: width,
            height:height,

            debugBreak: true,

            DebugBreak: function()
            {
                if (this.debugBreak)
                {
                    this.debugBreak = false;
                    debugger;
                }
            },
            gain: 1e-7,
            gainScale: 1,

            SetValue: function(value)
            {
                value = value*value;
                value = value*value*1000;
                if (value < 1E-7) value = 1E-7;

                this.gain = value;
                this.gainScale = 1/Math.atan(this.gain);
                this.MaybeDraw();
            },
            GainFn: function(x)
            {
                return Math.atan(x*this.gain)*this.gainScale;
            },

            Draw: function(svg)
            {
                svg.clear();

                var points = [];
                var ix = 0;
                var xs = this.width/2;
                var ys = (this.height-2)/2;
                var y0 = this.height/2;

                for (var x = -1.0; x <= 1.0; x += 2.0/this.width)
                {
                    var y = this.GainFn(x);
                    points[ix++] = [ (x+1)*xs, y0-y*ys];
                }
                svg.polyline(points,{ fill: 'none', stroke: '#880', strokeThickness: 0.5, opacity: 0.5});
            },

            MaybeDraw: function()
            {
                if (this.svgPlot)
                {
                    this.Draw(this.svgPlot);
                }
            },
            DrawPlot: function(svg)
            {
                this.svgPlot = svg;
                this.MaybeDraw();
            },
            Attach: function(selector)
            {
                //this.DebugBreak();
                this.plot = $("<div>");
                selector.append(this.plot);
                var self = this;
                this.plot.svg({onLoad: function (svg) { self.DrawPlot.apply(self,[svg]); } });
            }
        };

        result.Attach(selector);
        return result;
    }


};

