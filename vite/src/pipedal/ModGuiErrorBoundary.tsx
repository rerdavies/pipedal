/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */
import { Component, ErrorInfo, ReactNode } from "react";
import Typography from "@mui/material/Typography";
import Button from "@mui/material/Button";

import { UiPlugin } from "./Lv2Plugin";

interface Props {
    plugin: UiPlugin | null;
    children?: ReactNode;
    onClose: () => void;
}

interface State {
  hasError: boolean;
  message: string;
}

class ModGuiErrorBoundary extends Component<Props, State> {
  public state: State = {
    hasError: false,
    message: "",
  };

  public static getDerivedStateFromError(error: Error): State {
    // Update state so the next render will show the fallback UI.
    return { hasError: true, message: error.message };
  }

  public componentDidCatch(error: Error, errorInfo: ErrorInfo) {
    console.error("ModGUI rendering error: ", error);
    console.error("Stack trace: " , errorInfo.componentStack);
  }

  public render() {
    if (this.state.hasError) {
      return (
        <div style={{
            position: "absolute",left: 0, right: 0, top: 0, bottom: 0,
          display: "flex",
          flexDirection: "column",
          justifyContent: "stretch",
        }}>
            <div style={{ flex: "1 1 auto" }} />
            <div style={{display: "flex", flexDirection: "row", alignItems: "stretch", justifyContent: "stretch"}}>
                <div style={{ flex: "1 1 auto" }} />
                <div style={{maxWidth: 600, margin: 24}}>
                    <Typography variant="h6" style={{ marginBottom: 8 }} >
                        { this.props.plugin ?
                            `Error Loading '${this.props.plugin.name}'` :
                            `Error Loading UI`
                        }
                    </Typography>

                    <Typography variant="body2" style={{marginBottom:16}} >
                        {this.state.message}
                    </Typography>
                    <div style={{alignSelf: "flex-end", marginBottom: 16, marginRight: 24, display: "flex", flexFlow: "row nowrap", justifyContent: "flex-end"  }}>
                        <Button
                            style={{alignSelf: "flex-end"}}
                            variant="contained"
                            onClick={() => {
                                this.props.onClose();
                            
                            }}
                            >Close
                        </Button>  
                    </div>
                </div>
                <div style={{ flex: "1 1 auto" }} />
            </div>
            <div style={{ flex: "2 2 auto" }} />

        </div>
        
      );
    }

    return this.props.children;
  }
}

export default ModGuiErrorBoundary;
