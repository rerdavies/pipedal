/*
 *   Copyright (c) Robin E.R. Davies
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

import type { Root } from 'hast';

// Rewrite <a> tags with "t3klink?id=xxx" hrefs to call the T3kLinkHandler handler instead of navigating to a new page.

// // Hook in the rewriter as follows:
//                             <ReactMarkdown   urlTransform={urlTransform} rehypePlugins={
//                                     [
//                                         [remarkGfm],
//                                         [rehypeRaw],
//                                         [rehypeSanitize, extendedSchema],
//                                         [rehypeExternalLinks, {target: '_blank'}]
//                                         [rewriteT3kUrls, {...options}]
//                                     ]}>
//                                     {this.state.textFileContent}
//                                 </ReactMarkdown>

type HastNode = {
	type: string;
	tagName?: string;
	properties?: Record<string, unknown>;
	children?: HastNode[];
};

export type T3kLinkClickHandler = (href: string) => boolean;

export interface RewriteT3kUrlsOptions {
	prefixUrl?: string;
	onT3kLinkClick?: T3kLinkClickHandler;
}

// Default callback. Supply `onT3kLinkClick` in options to handle links.
export function T3kLinkHandler(href: string): boolean {
	void href;
	return false;
}


function getHref(properties: Record<string, unknown>): string | null {
	const href = properties.href;
	return typeof href === 'string' ? href : null;
}

function rewriteNode(node: HastNode, prefixUrl: string, onT3kLinkClick: T3kLinkClickHandler): void {
	if (node.type === 'element' && node.tagName === 'a') {
		const properties = node.properties ?? {};
		const href = getHref(properties);
		if (href && href.startsWith(prefixUrl)) {
			properties.target = '_blank';
			properties.rel = 'noopener noreferrer';
			properties.onClick = (event: { preventDefault?: () => void; stopPropagation?: () => void; }) => {
				const handled = onT3kLinkClick(href);
				if (handled) {
					event.preventDefault?.();
					event.stopPropagation?.();
				}
			};
			node.properties = properties;
		}
	}

	if (node.children) {
		for (const child of node.children) {
			rewriteNode(child, prefixUrl, onT3kLinkClick);
		}
	}
}


export function rewriteT3kUrls(options?: RewriteT3kUrlsOptions) {
	const prefixUrl = options?.prefixUrl;
	const onT3kLinkClick = options?.onT3kLinkClick ?? ((href: string) => T3kLinkHandler(href));
	return (tree: Root) => {
		if (!prefixUrl) {
			return;
		}
		rewriteNode(tree as unknown as HastNode, prefixUrl, onT3kLinkClick);
	};
}

