// Copyright (c) 2026 Thomas Rapolani
// SPDX-License-Identifier: MIT

import { useEffect, useMemo, useState } from "react";
import { Lv2PatchPropertyInfo } from "./Lv2Plugin";
import { ListenHandle, PiPedalModelFactory } from "./PiPedalModel";
import PluginControl from "./PluginControl";

interface PatchPropertyControlProps {
    instanceId: number;
    property: Lv2PatchPropertyInfo;
}

export default function PatchPropertyControl(props: PatchPropertyControlProps) {
    const model = PiPedalModelFactory.getInstance();
    const uiControl = useMemo(() => props.property.toUiControl(), [props.property]);
    const [value, setValue] = useState(uiControl.default_value);

    useEffect(() => {
        let mounted = true;
        let listenHandle: ListenHandle | undefined;

        if (props.property.readable) {
            listenHandle = model.monitorPatchProperty(
                props.instanceId,
                props.property.uri,
                (_instanceId, _propertyUri, nextValue) => {
                    if (mounted && typeof nextValue === "number") {
                        setValue(nextValue);
                    }
                }
            );
            model.getPatchProperty<number>(props.instanceId, props.property.uri)
                .then((nextValue) => {
                    if (mounted && typeof nextValue === "number") {
                        setValue(nextValue);
                    }
                })
                .catch(() => {
                    // Some plugins only report a value after the first patch:Set.
                });
        }

        return () => {
            mounted = false;
            if (listenHandle) {
                model.cancelMonitorPatchProperty(listenHandle);
            }
        };
    }, [model, props.instanceId, props.property.readable, props.property.uri]);

    const setPatchValue = (nextValue: number) => {
        setValue(nextValue);
        void model.setPatchProperty(props.instanceId, props.property.uri, nextValue)
            .catch((error) => {
                console.error(`Failed to set LV2 patch property ${props.property.uri}:`, error);
                if (props.property.readable) {
                    void model.getPatchProperty<number>(props.instanceId, props.property.uri)
                        .then((currentValue) => {
                            if (typeof currentValue === "number") {
                                setValue(currentValue);
                            }
                        });
                }
            });
    };

    return (
        <PluginControl
            instanceId={props.instanceId}
            uiControl={uiControl}
            value={value}
            onChange={setPatchValue}
            onPreviewChange={() => { }}
            requestIMEEdit={() => { }}
        />
    );
}
