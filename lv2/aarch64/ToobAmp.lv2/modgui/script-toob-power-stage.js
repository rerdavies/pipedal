method = function (event,jsFuncs) {


    function LoadScripts(ds)
    {
        var script = event.icon.find("#ToobFrequencyScript");

        $.get(script[0].src, function(data,status) {
                    ds.OnFrequencyScriptLoaded.apply(ds,[data,status]);
                });

        script = event.icon.find("#ToobVuScript");
        $.get(script[0].src, function(data,status) {
                    ds.OnVuScriptLoaded.apply(ds,[data,status]);
                });
        script = event.icon.find("#ToobGainScript");
        $.get(script[0].src, function(data,status) {
                    ds.OnGainScriptLoaded.apply(ds,[data,status]);
                });
    }


    
    function handle_event (symbol, value) {
        var ds;
        switch (symbol) {
        case "gain1":
            {
                ds = event.icon.data('xModFrequency');
                ds.SetGain(0,value);
            }
            break;
        case "gain2":
            {
                ds = event.icon.data('xModFrequency');
                ds.SetGain(1,value);
            }
            break;
        case "gain3":
            {
                ds = event.icon.data('xModFrequency');
                ds.SetGain(2,value);
            }
            break;
    
        case "boost":
            {
                ds = event.icon.data('xModFrequency');
                ds.SetBoost(value);
            }
            break;


        }
    }

    function MakeDs()
    {
        var ds = {

            UI_STATE_URI: "http://two-play.com/plugins/toob-power-stage#uiState",

            SpectrumDiv: event.icon.find('.toob_frequency_response'),

            icon: event.icon,


            attached: false,
            vuAttached:  false,
            gainAttached: false,

            OnGainScriptLoaded: function(data,status)
            {
                eval(data);
                if (ToobGain && !this.gainAttached)
                {


                    this.gainAttached = true;
                    this.ToobGain = ToobGain;
                    this.gainControls = [];

                    var divs =  event.icon.find('.toob_gain');
                    var self = this;
                    divs.each(function (i) 
                    {
                        self.gainControls[i] = self.ToobGain.CreateWrapper($(this),48,48);
                        self.gainControls[i].SetValue(self.gains[i]);
                    });

                }
            },
            gains: [1E-7,1E-7,1E-7],
            SetGain: function(gainStage,value)
            {
                this.gains[gainStage] = value;
                if (this.gainControls)
                {
                    this.gainControls[gainStage].SetValue(value);
                }
            },

            SAG_METER_SIZE:  { width: 2, height: 44},
            sagRects: [],

            DrawSagMeters: function(selector)
            {
                var self = this;
                var targetFill = { fill: "#044", stroke: 'none'};
                selector.each(function (i) {
                    var meter = $(this);
                    meter.svg({onLoad: function (svg) { 
                        self.sagRects[i] = svg.rect(0,0,
                                                    self.SAG_METER_SIZE.width,self.SAG_METER_SIZE.height,
                                                    0,0,
                                                    targetFill);
                    }});
                });
                $(this)
            },

            OnVuScriptLoaded: function(data,status)
            {
                eval(data);
                if (ToobVu && !this.vuAttached)
                {
                    this.vuAttached = true;
                    this.ToobVu = ToobVu;
                    this.vuControls = [];
                    var microVus = this.icon.find('.toob_micro_vu');
                    var self = this;
                    microVus.each(function (i) 
                    {
                        self.vuControls[i] = self.ToobVu.CreateWrapper($(this),1,20,66,-30,10,0,true);
                        self.vuControls[i].UpdateValue(self.vus[i]);
                    });

                    var masterVu = this.icon.find('.toob_large_vu');
                    this.vuControls[3] = this.ToobVu.CreateWrapper(masterVu,1,48,206,-80,10,12);
                    this.vuControls[3].UpdateValue(this.vus[3]);
                }
            },
            vuControls: null,
            vus: [0,0,0,0],

            SetVu: function(i,value)
            {
                this.vus[i] = value;
                if (this.vuControls)
                {
                    this.vuControls[i].UpdateValue(value);
                }
            },

            sag: 0,
            sagD: 0,

            SetSag: function(value)
            {
                this.sag = value;
                var rect = this.sagRects[0];
                if (rect != undefined)
                {
                    var y = this.SAG_METER_SIZE.height*this.sag;
                    rect.setAttribute("top",y,"");
                    rect.setAttribute("height",this.SAG_METER_SIZE.height-y,"");
                }
            },
            SetSagD: function(value)
            {
                
                this.sagD = value;
                var rect = this.sagRects[1];
                if (rect != undefined)
                {
                    var y = this.SAG_METER_SIZE.height*(this.sagD);
                    rect.setAttribute("top",y,"");
                    rect.setAttribute("height",this.SAG_METER_SIZE.height-y,"");
                }

            },
            OnFrequencyScriptLoaded: function(data,status)
            {
                eval(data);
                if (ToobFrequencyPlot && !ds.attached)
                {
                    ds.ToobFrequencyPlot = ToobFrequencyPlot;
                    ds.attached = true;
                    ds.frequencyPlot = ToobFrequencyPlot.CreateWrapper(this.SpectrumDiv,178,78);
                }
                this.MaybeDrawResponse();
            },
            MaybeDrawResponse: function() {
                if (this.frequencyPlot && this.values)
                {
                    this.frequencyPlot.SetValues(this.values);
                }
            },
            

            UpdateFrequencyResults: function(values)
            {
                this.values = values;
                this.MaybeDrawResponse();
            },
            UpdateUiState: function(values)
            {
                for (var i = 0; i < 4; ++i)
                {
                    this.SetVu(i,values[i]);
                }
                this.SetSag(values[4]);
                this.SetSagD(values[5]);
            }
        };
        ds.DrawSagMeters(event.icon.find(".toob-sag-meter"));
        return ds;

    }
    function handle_property(uri, value)
    {
        var ds = event.icon.data('xModFrequency');
        if (uri == ds.FREQUENCY_RESPONSE_URI)
        {
            ds.UpdateFrequencyResults(value);
        } else if (uri == ds.UI_STATE_URI)
        {
            ds.UpdateUiState(value);
        }
    }

    if (event.type == 'start') {
        debugger;

 
        // var dpy = event.icon.find('[mod-role=frequency-display]');

        var ds = MakeDs();
        event.icon.data('xModFrequency', ds);
        LoadScripts(ds);
        var ports = event.ports;
        for (var p in ports) {
            handle_event(ports[p].symbol, ports[p].value);
        }

    }
    else if (event.type == 'change') {
        if (event.symbol)
        {
            handle_event (event.symbol, event.value);
        } else {
            handle_property(event.uri,event.value);
        }
    } else {
        //debugger;
    }
};