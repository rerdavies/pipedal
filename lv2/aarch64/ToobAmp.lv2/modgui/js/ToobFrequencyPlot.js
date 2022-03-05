var ToobFrequencyPlot = {
    CreateWrapper: function (selector,width,height, dbMinOpt,dbMaxOpt,dbTickSpacingOpt)
    {
        if (dbMinOpt == undefined) dbMinOpt = -45;
        if (dbMaxOpt == undefined) dbMaxOpt = 5;
        if (dbTickSpacingOpt == undefined) dbTickSpacingOpt = 10;
        var result = { };

        result.MIN_DB_AF = Math.pow(10,-192/20);
        result.fMin = 30;
        result.fMax = 22050;
        result.logMin = Math.log(result.fMin);
        result.logMax = Math.log(result.fMax);

        // Size of the SVG element.
        result.xMin = 0;
        result.xMax = PLOT_WIDTH;
        result.yMin = 0;
        result.yMax = PLOT_HEIGHT;
        result.dbMin = dbMaxOpt; // deliberately reversed to flip up and down.
        result.dbMax = dbMinOpt;
        result.dbTickSpacing = dbTickSpacingOpt;

        result.DrawGrid = function(svg)
        {
            var style = { fill: 'none', stroke: "#066",strokeWidth: 0.25};

            for (var db = Math.ceil(this.dbMax/this.dbTickSpacing)*this.dbTickSpacing; db < this.dbMin; db += this.dbTickSpacing )
            {
                var y = (db-this.dbMin)/(this.dbMax-this.dbMin)*(this.yMax-this.yMin);
                svg.line(this.xMin,y,this.xMax,y,style);
            }
            var decade0 = Math.pow(10,Math.floor(Math.log10(this.fMin)));
            for (var decade = decade0; decade < this.fMax; decade *= 10)
            {
                for (var i = 0; i < 10; ++i)
                {
                    var f = decade*i;
                    if (f > this.fMin && f < this.fMax)
                    {
                        var x = this.xScale(f);
                        svg.line(x,this.yMin,x,this.yMax,style);
                    }
                }
            }
        };

        result.Draw = function(svg,values)
        {
            svg.clear();

            var points = new Array(values.length/2);
            for (var i = 0; i < values.length; i += 2)
            {
                var f = values[i];
                var v = values[i+1];
                var x = this.xScale(f);
                var y = this.afToY(v);
                points[i/2] = [x,y];
            }
            svg.polyline(points,{fill: 'none', stroke: "#990", strokeWidth: 0.5});
        };

        result.MaybeDraw = function()
        {
            if (this.values && this.svgPlot)
            {
                this.Draw(this.svgPlot,this.values);
            }
        };
        result.SetValues = function(values)
        {
            this.values = values;
            this.MaybeDraw();
        };

        result.DrawPlot = function(svg)
        {
            result.svgPlot = svg;
            result.MaybeDraw();
        };

        result.Attach = function(selector, width,height)
        {
            this.xMax = width;
            this.yMax = height;
            this.selector = selector;
            this.background = $("<div>");
            selector.append(this.background);
            this.plot = $("<div>");
            selector.append(this.plot);

            this.background.svg({onLoad: function (svg) { result.DrawGrid.apply(result,[svg]) } });
            this.plot.svg({onLoad: function (svg) { result.DrawPlot.apply(result,[svg]) } });


        };


        result.afToY = function(value)
        {
            value = Math.abs(value);

            var db;
            if (value < this.MIN_DB_AF) {
                db = -192.0;
            } else {
                db = 20*Math.log10(value);
            }
            var y = (db-this.dbMin)/(this.dbMax-this.dbMin)*(this.yMax-this.yMin) + this.yMin;
            return y;

        };

        result.xScale = function(frequency)
        {
            var logV = Math.log(frequency);
            return (this.xMax-this.xMin) * (logV-this.logMin)/(this.logMax-this.logMin) + this.xMin;
        };
        result.UpdateFrequencyResponse = function(values)
        {
            this.values = values;
            this.MaybeDrawResponse();
        };
        result.DrawResponse = function(svg,values)
        {
            svg.clear();

            var points = new Array(values.length/2);
            for (var i = 0; i < values.length; i += 2)
            {
                var f = values[i];
                var v = values[i+1];
                var x = this.xScale(f);
                var y = this.afToY(v);
                points[i/2] = [x,y];
            }
            svg.polyline(points,{fill: 'none', stroke: "#990", strokeWidth: 0.5});
        };

        result.Attach(selector,width,height);
        return result;
    }


};

