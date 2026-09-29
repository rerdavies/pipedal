import { PluginType, UiPlugin } from './Lv2Plugin';

export interface PluginCategory {
    id: string;
    label: string;
    color: string;
    rank: number;
}

const category = (
    id: string,
    label: string,
    color: string,
    rank: number,
): PluginCategory => ({ id, label, color, rank });

export const pluginCategories = {
    amp: category('amp', 'Amps', '#e49a36', 0),
    cab: category('cab', 'Cabs & IRs', '#c98942', 1),
    distortion: category('distortion', 'Distortion', '#e05a52', 2),
    dynamics: category('dynamics', 'Dynamics', '#d0b83f', 3),
    eq: category('eq', 'EQ', '#65ad5d', 4),
    modulation: category('modulation', 'Modulation', '#35a99a', 5),
    delay: category('delay', 'Delay', '#4f91d8', 6),
    reverb: category('reverb', 'Reverb', '#4c82c4', 7),
    pitch: category('pitch', 'Pitch & Synth', '#ad68cc', 8),
    filter: category('filter', 'Filter & Wah', '#7ab45c', 9),
    spatial: category('spatial', 'Spatial', '#d46f9e', 10),
    utility: category('utility', 'Volume & Utility', '#8d9aa7', 11),
    other: category('other', 'Other', '#8d9aa7', 12),
} satisfies Record<string, PluginCategory>;

export const orderedPluginCategories = Object.values(pluginCategories)
    .sort((left, right) => left.rank - right.rank);

export const CATEGORY_FILTER_PREFIX = 'category:';

// ---- User category overrides (persisted) -------------------------------
// Lets the user re-assign a mis-categorised plugin. Keyed by plugin URI ->
// category id. Persisted in localStorage so it survives reloads.
const CATEGORY_OVERRIDE_KEY = 'pipedal.pluginCategoryOverrides';

let categoryOverrides: Record<string, string> | null = null;

function loadCategoryOverrides(): Record<string, string> {
    if (categoryOverrides) return categoryOverrides;
    categoryOverrides = {};
    try {
        const raw = window.localStorage.getItem(CATEGORY_OVERRIDE_KEY);
        if (raw) {
            const parsed = JSON.parse(raw);
            if (parsed && typeof parsed === 'object') {
                categoryOverrides = parsed as Record<string, string>;
            }
        }
    } catch (_e) { /* ignore corrupt/unavailable storage */ }
    return categoryOverrides;
}

export function getPluginCategoryOverride(uri: string): PluginCategory | undefined {
    const id = loadCategoryOverrides()[uri];
    if (!id) return undefined;
    return orderedPluginCategories.find(c => c.id === id);
}

export function setPluginCategoryOverride(uri: string, categoryId: string | null): void {
    const overrides = loadCategoryOverrides();
    if (categoryId) {
        overrides[uri] = categoryId;
    } else {
        delete overrides[uri];
    }
    try {
        window.localStorage.setItem(CATEGORY_OVERRIDE_KEY, JSON.stringify(overrides));
    } catch (_e) { /* ignore */ }
}

export function getPluginCategory(pluginType: PluginType): PluginCategory {
    switch (pluginType) {
        case PluginType.NamPlugin:
        case PluginType.PiPedalAmpsNode:
        case PluginType.AmplifierPlugin:
        case PluginType.SimulatorPlugin:
            return pluginCategories.amp;

        case PluginType.DistortionPlugin:
        case PluginType.WaveshaperPlugin:
            return pluginCategories.distortion;

        case PluginType.DynamicsPlugin:
        case PluginType.CompressorPlugin:
        case PluginType.EnvelopePlugin:
        case PluginType.ExpanderPlugin:
        case PluginType.GatePlugin:
        case PluginType.LimiterPlugin:
            return pluginCategories.dynamics;

        case PluginType.EQPlugin:
        case PluginType.MultiEQPlugin:
        case PluginType.ParaEQPlugin:
            return pluginCategories.eq;

        case PluginType.FilterPlugin:
        case PluginType.AllpassPlugin:
        case PluginType.BandpassPlugin:
        case PluginType.CombPlugin:
        case PluginType.HighpassPlugin:
        case PluginType.LowpassPlugin:
            return pluginCategories.filter;

        case PluginType.ModulatorPlugin:
        case PluginType.ChorusPlugin:
        case PluginType.FlangerPlugin:
        case PluginType.PhaserPlugin:
            return pluginCategories.modulation;

        case PluginType.DelayPlugin:
            return pluginCategories.delay;

        case PluginType.ReverbPlugin:
            return pluginCategories.reverb;

        case PluginType.SpectralPlugin:
        case PluginType.PitchPlugin:
        case PluginType.GeneratorPlugin:
        case PluginType.ConstantPlugin:
        case PluginType.InstrumentPlugin:
        case PluginType.OscillatorPlugin:
            return pluginCategories.pitch;

        case PluginType.SpatialPlugin:
            return pluginCategories.spatial;

        case PluginType.UtilityPlugin:
        case PluginType.AnalyserPlugin:
        case PluginType.ConverterPlugin:
        case PluginType.FunctionPlugin:
        case PluginType.MixerPlugin:
            return pluginCategories.utility;

        default:
            return pluginCategories.other;
    }
}

export function getUiPluginCategory(plugin: UiPlugin): PluginCategory {
    const override = getPluginCategoryOverride(plugin.uri);
    if (override) {
        return override;
    }

    if (plugin.uri === 'http://two-play.com/plugins/toob-nam') {
        return pluginCategories.amp;
    }

    const name = `${plugin.name} ${plugin.plugin_display_type} ${plugin.author_name}`.toLowerCase();
    if (/\b(cab|cabinet|cab ir|ir loader)\b/.test(name)) {
        return pluginCategories.cab;
    }
    if (/\b(nam|neural amp|amplifier|amp model|preamp|power amp)\b/.test(name)) {
        return pluginCategories.amp;
    }
    if (/\b(reverb|verb|room|hall|plate|spring)\b/.test(name)) {
        return pluginCategories.reverb;
    }
    if (/\b(delay|echo|slapback)\b/.test(name)) {
        return pluginCategories.delay;
    }
    if (/\b(compressor|multi[\s-]?comp|limiter|gate|expander|de[\s-]?esser|transient|dynamics)\b/.test(name)) {
        return pluginCategories.dynamics;
    }
    if (/\b(eq|multi[\s-]?q|4k[\s-]?eq|equalizer|equaliser|tone stack)\b/.test(name)) {
        return pluginCategories.eq;
    }
    if (/\b(rotary|leslie|chorus|flanger|phaser|tremolo|vibrato)\b/.test(name)) {
        return pluginCategories.modulation;
    }
    if (/\b(tape|vinyl|warmer|saturat|crusher|overdrive|fuzz|distortion|drive)\b/.test(name)) {
        return pluginCategories.distortion;
    }
    if (/\b(pitch|harmoni[sz]er|octave|synth|oscillator)\b/.test(name)) {
        return pluginCategories.pitch;
    }
    if (/\b(wah|auto[\s-]?wah|filter|lowpass|highpass|bandpass)\b/.test(name)) {
        return pluginCategories.filter;
    }
    if (/\b(stereo tool|stereo width|spatial|mid[\s/]side)\b/.test(name)) {
        return pluginCategories.spatial;
    }
    if (/\b(loudness compensator|meter|analyser|analyzer|utility)\b/.test(name)) {
        return pluginCategories.utility;
    }
    return getPluginCategory(plugin.plugin_type);
}

export function getPluginCategoryTags(plugin: UiPlugin): string[] {
    const category = getUiPluginCategory(plugin);
    const result = [category.label];
    const subtype = plugin.plugin_display_type
        .replace(/\s*\((?:Stereo|Mono)\)\s*$/i, '')
        .trim();
    if (subtype && subtype.toLowerCase() !== category.label.toLowerCase()) {
        result.push(subtype);
    }
    if (plugin.audio_inputs === 2 || plugin.audio_outputs === 2) {
        result.push('Stereo');
    } else {
        result.push('Mono');
    }
    return Array.from(new Set(result));
}

export function getPluginTypeColor(pluginType: PluginType): string {
    return getPluginCategory(pluginType).color;
}
