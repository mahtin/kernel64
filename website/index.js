import { marked } from 'marked';
import readme from '../README.md';				// Wrangler supports bundling text assets
import theory from '../THEORY-OF-OPERATIONS.md';		// Wrangler supports bundling text assets
import changelog from '../CHANGELOG.md';			// Wrangler supports bundling text assets

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const topHtml = `
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Kernel64 - Project Documentation</title>
	<link rel="stylesheet" href="https://unpkg.com/simpledotcss/simple.min.css">
        <style>
          #kernel64logo { float: left; margin-top: 5px; margin-left: 20px; }
        </style>
      </head>
      <body class="markdown-body">
    `;
    const headerHtml = `
        <header>
          <a href="/"><img id='kernel64logo' src="kernel64logo-small.png"></a>
          <h1>Kernel64</h1>
          <p>A user level kernel for non-preemptive multiprocessing</p>
        </header>
    `;
    const bottomHtml = `
      </body>
      </html>
    ` + '<!-- "' + url.pathname + '" -->';

    let body_markup = "";
    // Switch based on the URL path
    switch (url.pathname) {
      case "/":
        body_markup = readme;
        body_markup = body_markup.replace("(README.md)", "(/)");
        body_markup = body_markup.replace("(THEORY-OF-OPERATIONS.md)", "(/theory.html)");
        body_markup = body_markup.replace("(CHANGELOG.md)", "(/changelog.html)");
        return new Response(topHtml + headerHtml + marked.parse(body_markup) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/theory.html":
        body_markup = theory;
        body_markup = body_markup.replace("(README.md)", "(/)");
        body_markup = body_markup.replace("(THEORY-OF-OPERATIONS.md)", "(/theory.html)");
        body_markup = body_markup.replace("(CHANGELOG.md)", "(/changelog.html)");
        return new Response(topHtml + headerHtml + marked.parse(body_markup) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/changelog.html":
        body_markup = changelog;
        body_markup = body_markup.replace(/\.\.\/\.\.\/commit\//g, "https://github.com/mahtin/kernel64/commit/");
        return new Response(topHtml + headerHtml + marked.parse(body_markup) + bottomHtml, { headers: { 'Content-Type': 'text/html;charset=UTF-8' }, });
      case "/kernel64logo-small.png":
        const image_small = await fetch("https://raw.githubusercontent.com/mahtin/kernel64/refs/heads/main/website/kernel64logo-small.png");
        const bytes_small = await image_small.bytes();
        return new Response(bytes_small, { headers: { 'Content-Type': 'image/png' }, });
      case "/kernel64logo-large.png":
        const image_large = await fetch("https://raw.githubusercontent.com/mahtin/kernel64/refs/heads/main/website/kernel64logo-large.png");
        const bytes_large = await image_large.bytes();
        return new Response(bytes_large, { headers: { 'Content-Type': 'image/png' }, });
      default:
        body_markup = "# Oops! Page Not Found \n\n";
	body.markup = body.markup + "We're sorry; but, the page you are looking for doesn't exist, has been removed, or its name has changed.\n\n";
	body.markup = body.markup + "Request: '" + url.pathname + "'id url\n\n";
	body.markup = body.markup + "[Return to home page](/)";
        return new Response(topHtml + headerHtml + marked.parse(body_markup) + bottomHtml, { status: 404 });		// Return a 404 Page Not Found for unmatched paths
    }
  },
};
