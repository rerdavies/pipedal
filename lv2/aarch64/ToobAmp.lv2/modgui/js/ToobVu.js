var ToobVu = {
    CreateWrapper: function (selector, _channels, width, height, minDb, maxDb, tickSpacingOpt, isTinyOpt)
    {
        if (tickSpacingOpt == undefined)
        {
            tickSpacingOpt = 6;
        }
        isTinyOpt = isTinyOpt == undefined? false: isTinyOpt;

        var result = { 
            MIN_DB_AF: Math.pow(10,-192/20),
            channels: _channels,
            dbMin: minDb,
            dbMax: maxDb,
            width: width,
            height:height,
            yMin: 0,
            yMax: height-4,
            xMin: 0,
            xMax: width,
            tickSpacing: tickSpacingOpt,
            isTiny: isTinyOpt,

            values: [-96],
            peaks: [-96],

            

            prepareLayout: function() {
                var ticksWidth;
                if (this.isTiny)
                {
                    this.tick0L = 0;
                    this.tick1R = width;
                    ticksWidth = this.tick1R-this.tick0L;
                    this.trackL = this.tick0L + ticksWidth/3;
                    this.trackR = this.tick0L + ticksWidth*2/3;
                    this.tick0R = this.trackL-2;
                    this.tick1L = this.trackR + 2;
                    this.ledL = this.trackL+1;
                    this.ledR = this.trackR-1;
                } else {
                    this.tick0L = 20;
                    this.tick1R = width;
                    ticksWidth = this.tick1R-this.tick0L;
                    if (this.channels > 1)
                    {
                        this.trackL = this.tick0L + ticksWidth/4;
                        this.trackR = this.tick0L + ticksWidth*3/4;

                    } else
                    {
                        this.trackL = this.tick0L + ticksWidth/3;
                        this.trackR = this.tick0L + ticksWidth*2/3;
                    }
                    this.tick0R = this.trackL-2;
                    this.tick1L = this.trackR + 2;
                    this.ledL = this.trackL+1;
                    this.ledR = this.trackR-1;
                }
            },

            DrawTicks: function (svg) {
                var majorTick = { fill: 'none', stroke: "#EEE",strokeWidth: 2, opacity: 0.5 };
                if (this.tickSpacing == 0)
                {
                    var y = this.dbToY(0);

                    svg.line(this.tick0L,y,this.tick0R,y,
                        majorTick);
                    if (!this.isTiny)
                    {
                        svg.line(this.tick1L,y,this.tick1R,y,
                            majorTick);
                    }
                } else {
                    var minorTick = { fill: 'none', stroke: "#EEE",strokeWidth: 1, opacity: 0.3 };
                    var textSettings = { fill: '#eee', stroke: 'none', fontFamily: '"cooper hewitt",Sans-Serif',fontSize: "6px", textAnchor: "end", opacity: 0.5};

                    for (var db = Math.ceil(this.dbMin/this.tickSpacing)*this.tickSpacing; db < this.dbMax; db += this.tickSpacing )
                    {
                        var y = this.dbToY(db);

                        svg.line(this.tick0L,y,this.tick0R,y,
                            db === 0? majorTick: minorTick);
                        if (!this.isTiny)
                        {
                            svg.line(this.tick1L,y,this.tick1R,y,
                                db === 0? majorTick: minorTick);

                            var text;
                            if (db > 0)
                            {
                                text = "+" + db;
                            } else {
                                text = "" + db;
                            }
                            svg.text(this.tick0L-2,y+2,text,textSettings);
                        }
                    }
                }
                var y0 = this.dbToY(0);
                var y1 = this.dbToY(-this.tickSpacing);
                var targetFill = {fill: '#FFF', stroke: 'none',opacity: 0.07};
                svg.rect(this.tick0L,y0,this.tick0R-this.tick0L,y1-y0,0,0,targetFill);
                svg.rect(this.tick1L,y0,this.tick1R-this.tick1L,y1-y0,0,0,targetFill);

                y0 = this.dbToY(this.dbMax);
                y1 = this.dbToY(this.dbMin);
                svg.rect(this.trackL,y0,this.trackR-this.trackL,y1-y0,0,0,{fill: '#000', stroke: 'none'});
            },

            greenVu: { fill: "#080", stroke: 'none',opacity: 0.5},
            redVu:  { fill: "#800", stroke: 'none',opacity: 0.5},
            yellowVu: { fill: "#880", stroke: 'none',opacity: 0.5},
            greenPeak: { fill: "#0F0", stroke: 'none',opacity: 1},
            redPeak:  { fill: "#F00", stroke: 'none',opacity: 1},
            yellowPeak: { fill: "#FF0", stroke: 'none',opacity: 1},
            greenPeakColor: "#0F0",
            nonePeakColor: "transparent",
            redPeakColor: "#F00",
            yellowPeakColor: "#FF0",

            debugBreak: true,

            DebugBreak: function()
            {
                if (this.debugBreak)
                {
                    this.debugBreak = false;
                    debugger;
                }
            },
            Update: function()
            {
                var yMin = this.dbToY(this.dbMin);
                var yMax = this.dbToY(this.dbMax);
                var yZero = this.dbToY(0);
                var yYellow = this.dbToY(-6);

                for (var channel = 0; channel < this.channels; ++channel)
                {
                    var yValue = this.dbToY(this.values[channel]);
                    var yPeak = this.dbToY(this.peaks[channel]);

                    var channelControls = this.channelControls[channel];
                    if (yValue < yMin)
                    {
                        var yGreen = Math.max(yYellow,yValue);
                        channelControls.greenRect.setAttribute("y",yGreen,"");
                        channelControls.greenRect.setAttribute("height",yMin-yGreen,"");
                    } else {
                        channelControls.greenRect.setAttribute("y",0,"");
                        channelControls.greenRect.setAttribute("height",0,"");
                    }
                    if (yValue < yYellow)
                    {
                        var y = Math.max(yZero,yValue);
                        channelControls.yellowRect.setAttribute("y",y,"");
                        channelControls.yellowRect.setAttribute("height",yYellow-y,"");
                    } else {
                        channelControls.yellowRect.setAttribute("y",0,"");
                        channelControls.yellowRect.setAttribute("height",0,"");

                    }
                    if (yValue < yZero)
                    {
                        var y = Math.max(yMax,yValue);

                        channelControls.redRect.setAttribute("y",y,"");
                        channelControls.redRect.setAttribute("height",yZero-y,"");
                    } else {
                        channelControls.redRect.setAttribute("y",0,"");
                        channelControls.redRect.setAttribute("height",0,"");
                    }
                    var peakFill = null;
                    var peakHeight = 2;
                    if (yPeak < yZero)
                    {
                        peakFill = this.redPeakColor;
                    } else if (yPeak < yYellow)
                    {
                        peakFill = this.yellowPeakColor;
                    } else if (yPeak < yMin)
                    {
                        peakFill = this.greenPeakColor;
                    } else {
                        peakFill = this.nonePeakColor;
                        peakHeight = 0;
                    }
                    {
                        var y = Math.max(yMax+1,yPeak);
                        channelControls.peakRect.setAttribute("y",y-1,"");
                        channelControls.peakRect.setAttribute("height",peakHeight);
                        channelControls.peakRect.setAttribute("fill",peakFill);
                    }
                }
                
                if (!this.isTiny && this.hasGate)
                {
                    // 0: disabled, 1: released, 2: releasing, 3: attacking, 4: holding.
                    var yGateThreshold = this.dbToY(this.gateThreshold);
                    var showGate = (this.gateEnabled && yGateThreshold < yMin);

                    if (showGate)
                    {
                        var state = { fill: this.gateState >= 3 ? "#080" : "#C00", stroke: '#000', strokeWidth: 1};
                        var fill = this.gateState >= 3 ? "#080" : "#C00";

                        this.gateIndicatorL.setAttribute("fill",fill,"");
                        this.gateIndicatorR.setAttribute("fill",fill,"");
                        var pointsL = (this.trackL-1) +","+ (yGateThreshold) +
                                 " " + (this.trackL-9) + "," + (yGateThreshold+4) +
                                 " " + (this.trackL-9) + "," + (yGateThreshold-4) + 
                                 " " + (this.trackL-1) +","+ (yGateThreshold);
                        this.gateIndicatorL.setAttribute("points",pointsL,"");

                        var pointsR = (this.trackR+1) +","+ (yGateThreshold) +
                                " " + (this.trackR+9) + "," + (yGateThreshold-4) +
                                " " + (this.trackR+9) + "," + (yGateThreshold+4) +
                                " " + (this.trackR+1) +","+ (yGateThreshold);
                        this.gateIndicatorR.setAttribute("points",pointsR,"");

                        this.gateIndicatorL.setAttribute("opacity","1","");
                        this.gateIndicatorR.setAttribute("opacity","1","");
                    } else {
                        this.gateIndicatorL.setAttribute("opacity","0","");
                        this.gateIndicatorR.setAttribute("opacity","0","");
                    }
                }

            },
            Draw: function(svg)
            {
                svg.clear();

                for (var channel = 0; channel < this.channels; ++channel)
                {
                    this.channelControls[channel] = this.drawChannel(svg,channel);
                }
                if (!this.isTiny)
                {
                    var yGateThreshold = this.dbToY(0);
                    var state = { fill: "transparent", stroke: '#000', strokeWidth: 1};
                    var points = [[0,0]]; // just create the structure. Updates will give the shape.
                    this.gateIndicatorL = svg.polyline(points,state);
                    this.gateIndicatorR = svg.polyline(points,state);
                }
            },
            drawChannel: function(svg,channel)
            {
                var result = {};
                var width = ((this.ledR-this.ledL)-(this.channels-1))/this.channels;
                var x0 = this.ledL + channel*(width+1);
                

                // render all the controls, without y info which will be filled in during Update.

                result.greenRect = svg.rect(x0,0,width,0,0,0,this.greenVu);
                result.yellowRect = svg.rect(x0,0,width,0,0,0,this.yellowVu);
                result.redRect = svg.rect(x0,0,width,0,0,0,this.redVu);
                result.peakRect = svg.rect(x0,0,width,0,0,0,this.greenPeak);
                return result;
            },

            channelControls: [],
            MaybeUpdate: function()
            {
                if (this.svgPlot)
                {
                    for (var i = 0; i < this.channels; ++i)
                    {
                        this.Update(this.channelControls[i],this.values[i],this.peaks[i]);
                    }
                }
            },
            getTimeMs: function()
            {
                return new Date().getTime();
            },
            lastPeaks: [-96],
            holdTimes: [0],
            
            holdLengthMs: 2000,
            holdTime: 0,
            peakDecayRate: -96/4000,
            valueDecayRate: -96/1000,
            lastValueTime: 0,
            gateState: 0,

            UpdateValue: function(value, gateStateOpt)
            {
                this.UpdateValues([value],gateStateOpt);
            },
            UpdateValues: function(values,gateStateOpt)
            {
                if (gateStateOpt == undefined)
                {
                    gateStateOpt = 0;
                }
                //this.DebugBreak();
                var time = this.getTimeMs();

                for (var i = 0; i < this.values.length; ++i)
                {
                    var value = i < values.length? values[i]: 0;
                    value = this.afToDb(value); 
                    var decayedValue = this.values[i] +(time-this.lastValueTime)*this.valueDecayRate;
                    if (decayedValue > value)
                    {
                        value = decayedValue;
                    }
                    this.values[i] = value;


                    if (time > this.holdTimes[i])
                    {
                        this.peaks[i] = this.lastPeaks[i] +(time-this.holdTimes[i])*this.peakDecayRate;
                    }
                    if (value > this.peaks[i])
                    {
                        this.lastPeaks[i] = this.peaks[i] = value;
                        this.holdTimes[i] = time+this.holdLengthMs;
                    }
                }
                this.lastValueTime = time;
                this.gateState = gateStateOpt;
                this.MaybeUpdate();
            },

            gateEnabled: false,
            gateThreshold: -180,
            hasGate: false,
            SetGateEnabled: function(enabled)
            {
                this.hasGate = true;
                if (this.gateEnabled != enabled)
                {
                    this.gateEnabled = enabled;
                    this.MaybeUpdate();
                }
            },


            SetGateThreshold: function(value)
            {
                this.hasGate = true;
                if (this.gateThreshold != value)
                {
                    this.gateThreshold = value;
                    this.MaybeUpdate();
                }
            },
            DrawPlot: function(svg)
            {
                this.svgPlot = svg;
                this.Draw(this.svgPlot,this.value,this.peak);
            },
            dbToY: function(db)
            {
                if (db < this.minDb) db = this.minDb;
                if (db > this.maxDb) db = this.maxDb;
                return (db-this.dbMin)/(this.dbMax-this.dbMin)*(this.yMin-this.yMax)+this.yMax;
            },
            afToDb: function(af)
            {
                var value = Math.abs(af);
    
                var db;
                if (value < this.MIN_DB_AF) {
                    db = -192.0;
                } else {
                    db = 20*Math.log10(value);
                }
                return db;
            },

            afToY: function(value)
            {
                var db = afToDb(value);
                return this.dbToY(db);
            },
    

            Attach: function(selector)
            {

                var time = this.getTimeMs();
                this.lastValueTime = time;
                if (this.holdTime === 0) this.holdTime = time;
                for (var i = 1; i < this.channels; ++i)
                {
                    this.values[i] = this.values[0];
                    this.peaks[i] = this.peaks[0];
                    this.lastPeaks[i] = this.peaks[0];
                }
                for (var i = 0; i < this.channels; ++i)
                {
                    this.holdTimes[i] = time;
                }
                this.prepareLayout();
                this.background = $("<div>");
                selector.append(this.background);
                this.plot = $("<div>");
                selector.append(this.plot);
                var self = this;
                this.background.svg({onLoad: function (svg) { self.DrawTicks.apply(self,[svg]); } });
                this.plot.svg({onLoad: function (svg) { self.DrawPlot.apply(self,[svg]); } });
            }
    
        };

        result.Attach(selector);
        return result;
    }


};

