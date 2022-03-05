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
    }


    
    function handle_event (symbol, value) {
        switch (symbol) {
        case "gate_t":
            {
                var ds = event.icon.data('xModFrequency');
                ds.SetGateThreshold(value);
                ds.SetGateEnabled(value != -80);
            }
            break;
        case "boost":
            {
                var ds = event.icon.data('xModFrequency');
                ds.SetBoost(value);
            }
            break;


        }
    }

    function MakeDs()
    {
        var ds = {

            FREQUENCY_RESPONSE_URI: "http://two-play.com/plugins/toob#frequencyResponseVector",
            UI_STATE_URI: "http://two-play.com/plugins/toob-cab-sim#uiState",

            SpectrumDiv: event.icon.find('.toob_cabsim_fresp'),


            attached: false,
            vuAttached:  false,


            OnVuScriptLoaded: function(data,status)
            {
                eval(data);
                if (ToobVu && !this.vuAttached)
                {
                    this.vuAttached = true;
                    this.ToobVu = ToobVu;
                    this.vu = this.ToobVu.CreateWrapper(this.VuDiv,2,48,130,-80,20,12);
                }
            },
            gateThreshold: -192,
            gateEnabled: false,
            SetGateThreshold: function(value)
            {
                this.gateThreshold = value;
                if (this.vu)
                {
                    this.vu.SetGateThreshold(value);
                }
            },
            SetGateEnabled: function(enabled)
            {
                this.gateEnabled = enabled;
                if (this.vu)
                {
                    this.vu.SetGateEnabled(enabled);
                }
            },
            boost: 0,
            SetBoost: function(value)
            {
                this.boost = value;
                if (this.gain)
                {
                    this.gain.SetValue(value);
                }
            },
            OnFrequencyScriptLoaded: function(data,status)
            {
                eval(data);
                if (ToobFrequencyPlot && !ds.attached)
                {
                    ds.ToobFrequencyPlot = ToobFrequencyPlot;
                    ds.attached = true;
                    ds.frequencyPlot = ToobFrequencyPlot.CreateWrapper(this.SpectrumDiv,236,66);
                }
                this.MaybeDrawResponse();
            },
            MaybeDrawResponse: function() {
                if (this.frequencyPlot && this.values)
                {
                    this.frequencyPlot.SetValues(this.values);
                }
            },
            VuDiv: event.icon.find('.cabsim_vu'),

            UpdateFrequencyResults: function(values)
            {
                this.values = values;
                this.MaybeDrawResponse();
            },
            UpdateUiState: function(values)
            {
                var vuLevel = values[0];
                var gateState = values[1];
                if (this.vu)
                {
                    this.vu.UpdateValues([ values[0], values[1] ],gateState);
                }
            }
        };
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
        //debugger;

 
        // var dpy = event.icon.find('[mod-role=frequency-display]');

        var ds = MakeDs();
        event.icon.data('xModFrequency', ds);
        LoadScripts(ds);
        var ports = event.ports;
        for (var p in ports) {
            handle_event(ports[p].symbol, ports[p].value);
        }
        // refresh the frequency display if not first load.
        jsFuncs.patch_get(ds.FREQUENCY_RESPONSE_URI);

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