
import { useState } from "react";
import { PluginType } from "./Lv2Plugin";
import PluginIcon, { getIconColorSelections } from './PluginIcon';
import IconColorDropdownButton, {DropdownAlignment} from "./IconColorDropdownButton";

function IconTest() {
    const [selectedColor, setSelectedColor] = useState<string | undefined>(undefined);
    const [selectedKey, setSelectedKey] = useState<string>("");
    return (
        <>
            <div style={{ display: "flex", flexFlow: "row wrap", rowGap: 8, columnGap: 8, padding: 16, background: "#282828" }}>
                {
                    getIconColorSelections().map((color) => {
                        return (
                            <div key={color.key} style={{ width: 48, height: 48, background: color.value, borderRadius: 4,opacity: 0.8 }} 
                                onClick={() => { 
                                    setSelectedColor(color.value ); 
                                }}
                            />
                        );

                    })
                }

            </div>
            <div style={{ display: "flex", flexFlow: "row wrap", rowGap: 8, columnGap: 8, padding: 16, background: "#E8E8E8" }}>
                {
                    getIconColorSelections(false).map((color) => {
                        return (
                            <div key={color.key} style={{ width: 48, height: 48, background: color.value, borderRadius: 4,opacity: 0.8 }} 
                                onClick={() => { setSelectedColor(color.value ); }}
                            />
                        );

                    })
                }

            </div>
            <div>
                <IconColorDropdownButton 
                    colorKey={selectedKey} 
                    onColorChange={(selection) => 
                        {
                            setSelectedColor(selection.value);
                            setSelectedKey(selection.key);
                        } 
                    }
                    dropdownAlignment={DropdownAlignment.SE} />
            </div>
            <div style={{ display: "flex", flexFlow: "row wrap", rowGap: 8, columnGap: 8, padding: 16 }}>
                {
                    Object.values(PluginType).map((pluginType) => (
                        <div key={pluginType}>
                            <PluginIcon pluginType={pluginType} size={32} opacity={0.8} color={selectedColor} />
                        </div>
                    ))
                }
            </div>
            <div style={{ display: "flex", flexFlow: "row wrap", rowGap: 8, columnGap: 8, padding: 16, background: "#E8E8E8" }}>
                {
                    Object.values(PluginType).map((pluginType) => (
                        <div key={pluginType}>
                            <PluginIcon pluginType={pluginType} size={32} opacity={0.8} color={selectedColor} />
                        </div>
                    ))
                }
            </div>
        </>
    );
}

export default IconTest;