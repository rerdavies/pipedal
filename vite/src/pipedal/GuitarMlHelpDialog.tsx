import DialogEx from "./DialogEx";
import DialogTitle from "@mui/material/DialogTitle";
import DialogContent from "@mui/material/DialogContent";
import Toolbar from "@mui/material/Toolbar";
import IconButtonEx from "./IconButtonEx";
import ArrowBackIcon from "@mui/icons-material/ArrowBack";
import Typography from "@mui/material/Typography";
import Link from "@mui/material/Link";



function GuitarMlHelpDialog(props: {
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
                            The <Link href="https://www.tone3000.com" target="_blank">GuitarML Tone Library Project</Link> provides 
                            a substantial collection of high-quality neural amp models for use in plugins that use the ML 
                            model file format.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            To use GuitarML models with TooB ML, you must first download the collection of models from the Guitar ML website
                            by clicking on the link. This should download a file named "Proteus_Tone_Packs.zip" into your browser's Downloads directory.
                            You must then upload the downloaded zip file, found in your browser's Download directory to the
                            Pipedal server using the <em>Upload</em> button in PiPedal's file browser dialog.
                            PiPedal automatically extracts bundles of model files from zip archives, so manual extraction is unnecessary.
                        </Typography>
                        <Typography variant="body1" component="div" display="block">
                            Please consider <Link href="https://www.patreon.com/GuitarML" target="_blank">sponsoring the GuitarML project</Link> in order to support the ongoing development of the Tone Library.
                        </Typography>
                    </div>
                    <div style={{ flex: "2 2 1px" }} />
                </div>


            </DialogContent>
        </DialogEx>
    );
}
export default GuitarMlHelpDialog;