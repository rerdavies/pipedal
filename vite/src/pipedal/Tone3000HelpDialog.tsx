import DialogEx from "./DialogEx";
import DialogTitle from "@mui/material/DialogTitle";
import DialogContent from "@mui/material/DialogContent";
import Toolbar from "@mui/material/Toolbar";
import IconButtonEx from "./IconButtonEx";
import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import Typography from "@mui/material/Typography";
import Link from "@mui/material/Link";



function Tone3000HelpDialog(props: {
    open: boolean;
    onClose: () => void;
}) {
    const { open, onClose } = props;
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

                    <Typography id="tone3000-auth-dialog-title" noWrap component="div" sx={{ flexGrow: 1 }}>
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
                        <Typography variant="body1" component="div" display="block" >
                            The <Link href="https://www.tone3000.com" target="_blank">TONE3000 website</Link> provides a
                            massive collection of neural amp models that can be used by TooB Neural Amp Modeler.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            Using TONE3000 model files in PiPedal is a two step process. First, download model files from 
                            the website using your browser; and then upload
                            the files in your browser's Downloads directory to the PiPedal server using 
                            the <strong><em>Upload</em></strong> button. PiPedal 
                            automatically extracts bundles of model files from zip archives, so manual extraction is unnecessary.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            TONE3000 is a community-driven platform where community members share Neural Amp Modeler profiles. 
                            The site showcases the collaborative spirit of the guitar modeling community. And all of this is made 
                            possible by Steven Atkins' <Link href="https://www.neuralampmodeler.com/">Neural Amp Modeler library</Link>, which  forms the foundation and core of TooB Neural Amp Modeler and other NAM-based plugins.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            If you would like to profile your own amplifiers, and effects, the TONE3000 website allows you to 
                            generate your own NAM profiles for free. Refer to 
                            the <Link href="https://www.tone3000.com/capture">TONE3000 website</Link> for more details.
                            </Typography>
                        <Typography variant="body1" component="div" display="block" >
                            When you click on the TONE3000 link, you will be taken to an external website. The TONE3000 
                            website is not part of PiPedal, and is not affiliated in any way with PiPedal. 
                        </Typography>

                        <Typography variant="body2" component="div" display="block" style={{ marginTop: 32 }}>
                            Privacy statement: PiPedal has no access to personal data used by the TONE3000 website. Please refer to
                            the <Link href="https://www.tone3000.com/privacy" target="_blank">TONE3000 privacy policy</Link> for
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