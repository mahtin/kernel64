import { marked } from 'marked';
import readme from '../README.md';				// Wrangler supports bundling text assets
import theory from '../THEORY-OF-OPERATIONS.md';		// Wrangler supports bundling text assets
import changelog from '../CHANGELOG.md';			// Wrangler supports bundling text assets

export default {
  async fetch(request, env, ctx) {
    const topHtml = `
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Kernel64 - Project Documentation</title>
        <!-- Simple.css styling - Minified version -->
	<link rel="stylesheet" href="https://unpkg.com/simpledotcss/simple.min.css">
        <style>
	  /*
          body { box-sizing: border-box; min-width: 200px; max-width: 980px; margin: 0 auto; padding: 45px; }
          @media (max-width: 767px) { body { padding: 15px; } }
	  */
          #kernel64logo { float: left; margin-top: 5px; margin-left: 20px; }
        </style>
      </head>
      <body class="markdown-body">
        <header>
          <img id='kernel64logo' src="kernel64logo-small.png">
          <h1>Kernel64</br>Project Documentation</h1>
          <p>A user level kernel for non-preemptive multiprocessing</p>
        </header>
    `;
    const bottomHtml = `
      </body>
      </html>
    `;

    // Switch based on the URL path
    switch (request.pathname) {
      case "/":
        return new Response(topHtml + marked.parse(readme) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/readme.html":
        return new Response(topHtml + marked.parse(readme) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/theory.html":
        return new Response(topHtml + marked.parse(theory) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/changelog.html":
        return new Response(topHtml + marked.parse(changelog) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "kernel64logo-small.png":
        const image = await fetch("https://raw.githubusercontent.com/mahtin/kernel64/refs/heads/main/website/kernel64logo-small.png");
        const bytes = await image.bytes();
        return new Response(bytes, { headers: { 'Content-Type': 'image/png' }, });
      default:
        return new Response(topHtml + marked.parse(readme) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
//      return new Response("Page Not Found", { status: 404 });		// Return a 404 Page Not Found for unmatched paths
    }
  },
};
