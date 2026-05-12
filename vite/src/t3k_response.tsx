//import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import React from 'react';


import './index.css'

// createRoot(document.getElementById('root')!).render(
//   <StrictMode>
//     <App />
//   </StrictMode>,
// )

function ResponseComponent() {

    React.useEffect(() => {
        alert("yyy: Delete me");
        debugger;
        if (window.opener === null) {
            console.error('No window.opener found for OAuth callback');
            window.close();
            return;
        }
        window.opener.postMessage(
            {
                type: "t3k_response",
                uri: window.location.href,
                storedState: sessionStorage.getItem('t3k_state'),
                codeVerifier: sessionStorage.getItem('t3k_code_verifier')   
            }, '*'
        );

    }, []);
    
    return <div style={{color: 'white'}}>Processing...</div>;

}
createRoot(document.getElementById('root')!).render(
    <div>
        <ResponseComponent />
    </div>
)
