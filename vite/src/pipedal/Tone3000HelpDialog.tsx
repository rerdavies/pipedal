import DialogEx from "./DialogEx";
import DialogTitle from "@mui/material/DialogTitle";
import DialogContent from "@mui/material/DialogContent";
import Toolbar from "@mui/material/Toolbar";
import IconButtonEx from "./IconButtonEx";
import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import Typography from "@mui/material/Typography";
import Link from "@mui/material/Link";
import Tone3000DownloadType from "./Tone3000DownloadType";


function Tone3000HelpDialog(props: {
    open: boolean;
    downloadType: Tone3000DownloadType
    onClose: () => void;
}) {
    const { open, downloadType, onClose } = props;
    return (
        <DialogEx
            tag="tone3000-help"
            fullWidth={true}
            maxWidth="sm"
            onEnterKey={() => { onClose(); }}
            onClose={() => { onClose(); }}
            open={open}>

            <DialogTitle >
                <Toolbar style={{ padding: 0 }}>
                    <IconButtonEx
                        tooltip="Close"
                        edge="start"
                        color="inherit"
                        aria-label="cancel"
                        style={{ opacity: 0.6 }}
                        onClick={() => { onClose(); }}
                    >
                        <ArrowBackIcon style={{ width: 24, height: 24 }} />
                    </IconButtonEx>

                    <Typography id="tone3000-help-dialog-title" noWrap component="div" sx={{ flexGrow: 1 }}>
                        TONE3000 Help
                    </Typography>

                </Toolbar>
            </DialogTitle>
            <DialogContent>
                <div style={{ display: "flex", flexFlow: "row nowrap", alignItems: "start" }}>
                    <div style={{ flex: "1 1 1px" }} />
                    <div style={{
                        maxWidth: 500, marginLeft: "auto", marginRight: "auto",
                        display: "flex", flexFlow: "column nowrap", gap: 16, alignItems: "start",
                    }}>
                        {downloadType === Tone3000DownloadType.CabIr && (
                            <>
                                <Typography variant="body1" component="div" display="block" >
                                    The <Link href="https://www.tone3000.com" target="_blank">TONE3000 website</Link> provides a
                                    massive collection of high-quality Neural Amp Modeler models that can be used by TooB Neural Amp Modeler. It also 
                                    provides collections of cabinet Impulse Response (I/R) files that can be used with <i>TooB Cab IR</i>.
                                </Typography>
                                <Typography variant="body1" component="div" display="block">
                                    When you click on the button to download I/R files from TONE3000,
                                    selected I/R bundles will be directly downloaded from the TONE3000 website and
                                    uploaded to your PiPedal server. For this to work, the server must have
                                    internet access, and your client (Android app or browser) must have internet
                                    access as well.</Typography>
                                <Typography variant="body1" component="div" display="block">
                                    If either of your devices does not have internet access, you can still use TONE3000.
                                    Use a browser on a device with internet access to download model files by connecting to
                                    <Link href="https://www.tone3000.com" target="_blank">https://www.tone3000.com</Link> and
                                    download model packs directly to your local device. You can then upload the model .zip
                                    files to the PiPedal Server using the <strong><em>Upload</em></strong> button in the
                                    <strong><em>File Properties</em></strong> dialog in the PiPedal app or web interface. 
                                    PiPedal will extract I/R files from the .zip file bundles automatically, so there is no 
                                    need to extract them first.
                                </Typography>
                            </>
                        )}
                        {downloadType === Tone3000DownloadType.Nam && (
                            <>
                                <Typography variant="body1" component="div" display="block" >
                                    The <Link href="https://www.tone3000.com" target="_blank">TONE3000 website</Link> provides a
                                    massive collection of high-quality Neural Amp Modeler files that can be used by TooB Neural Amp Modeler to 
                                    perform high-quality simulations of amps and effect pedals.
                                </Typography>
                                <Typography variant="body1" component="div" display="block">
                                    When you click on the button to download Neural Amp Modeler files from TONE3000,
                                    model file bundles will be directly downloaded from the TONE3000 website and
                                    uploaded to your PiPedal server. For this to work, the server must have
                                    internet access, and your client (Android app or browser) must have internet
                                    access as well.</Typography>
                                <Typography variant="body1" component="div" display="block">
                                    Neural Amp Modeler models that simulate amp heads will typically need a speaker/cabinet simulation to 
                                    sound their best, or of course, need to be played through an actual physical amp speaker cabinet. The TONE3000 
                                    website provides a broad selection of cabinet I/R files that can be used
                                    with <i>TooB Cab IR</i> to provide high-quality cabinet simulation for your NAM models. <i>TooB Cab IR</i> allows 
                                    you to browse and download I/R files from TONE3000 directly within the PiPedal app. 
                                </Typography>

                                <Typography variant="body1" component="div" display="block">
                                    If either of your devices does not have internet access, you can still use TONE3000.
                                    Use a browser on a device with internet access to download Neural Amp Modeler files by connecting to
                                    <Link href="https://www.tone3000.com" target="_blank">https://www.tone3000.com</Link> and
                                    download Neural Amp Modeler file packs directly to your local device as .zip file bundles. You can then 
                                    upload the .zip file bundle to the PiPedal Server using the <strong><em>Upload</em></strong> button in the
                                    <strong><em>File Properties</em></strong> dialog. PiPedal will 
                                    extract model files from the .zip archives automatically, so there is no need to extract them first.
                                </Typography>
                            </>
                        )}
                        <Typography variant="body1" component="div" display="block">
                            TONE3000 is a community-driven platform where community members share Neural Amp Modeler profiles.
                            The site showcases the collaborative spirit of the guitar modeling community, made
                            possible by Steven Atkins' <Link href="https://www.neuralampmodeler.com/">Neural Amp Modeler library</Link>, which forms the foundation and 
                            core of TooB Neural Amp Modeler and other NAM-based plugins.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            If you would like to profile your own amplifiers and effects, the TONE3000 website allows you to
                            generate your own NAM profiles for free. Refer to
                            the <Link href="https://www.tone3000.com/capture" target="_blank">TONE3000 Capture page</Link> for more details.
                        </Typography>
                        <Typography variant="body1" component="div" display="block" >
                            When you click on the TONE3000 link, you will be taken to an external website. The TONE3000
                            website is not part of PiPedal and is not affiliated in any way with PiPedal.
                        </Typography>

                        <Typography variant="body2" component="div" display="block" style={{ marginTop: 32 }}>
                            Privacy statement: PiPedal has no access to personal data used by the TONE3000 website. Please refer to
                            the <Link href="https://www.tone3000.com/privacy" target="_blank">TONE3000 Privacy Policy</Link> for
                            information on how your data is used by TONE3000.
                        </Typography>
                    </div>
                    <div style={{ flex: "2 2 1px" }} />
                </div>


            </DialogContent>
        </DialogEx>
    );
}
export default Tone3000HelpDialog;