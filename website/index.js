//
// Kernel64 website via Cloudflare workers (but not using Pages yet)
//

import { marked } from 'marked';

// Wrangler supports bundling text assets (must place in wranger.json file "rules"/"globs")
import readme from '../README.md'
import theory from '../THEORY-OF-OPERATIONS.md';
import changelog from '../CHANGELOG.md';
import simple_min_css from './simple.min.css' with { type: 'css' };

const HTML_TOP = `
  <!DOCTYPE html>
  <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Kernel64 - Project Documentation</title>
      <link rel="stylesheet" href="/simple.min.css">
      <style>
        #kernel64logo { float: left; margin-top: 5px; margin-left: 20px; }
      </style>
      <link rel="shortcut icon" href="/favicon.ico" type="image/x-icon" />
      <link rel="apple-touch-icon" href="/kernel64logo-180x180.png" sizes="152x152" type="image/png" />
      <!-- UNUSED PRESENTLY
        <link rel="apple-touch-icon" sizes="180x180" href="/apple-touch-icon.png">
        <link rel="icon" type="image/png" sizes="32x32" href="/favicon-32x32.png">
        <link rel="icon" type="image/png" sizes="16x16" href="/favicon-16x16.png">
        <link rel="manifest" href="/site.webmanifest">
      -->
    </head>
    <body class="markdown-body">
`;

const HTML_HEADER_SECTION = `
      <header>
        <a href="/"><img id='kernel64logo' src="/kernel64logo-small.png"></a>
        <h1>Kernel64</h1>
        <p>A user level kernel for non-preemptive multiprocessing</p>
      </header>
`;
const HTML_BOTTOM = `
    </body>
  </html>
`;

function replace_markup(r) {
  r = r.replace("(README.md)", "(/)");
  r = r.replace("(THEORY-OF-OPERATIONS.md)", "(/theory.html)");
  r = r.replace("(CHANGELOG.md)", "(/changelog.html)");
  r = r.replace("website/kernel64logo", "/kernel64logo");
  r = r.replace(/\.\.\/\.\.\/commit\//g, "https://github.com/mahtin/kernel64/commit/");
  return r;
}

function html_from_markup(markup) {
  return HTML_TOP + HTML_HEADER_SECTION + marked.parse(replace_markup(markup)) + HTML_BOTTOM;
}

const MY_GITHUB_PROJECT_BASE = "https://raw.githubusercontent.com/mahtin/kernel64/refs/heads/main/website";

async function fetch_image_from_github(pathname) {
  const image = await fetch(MY_GITHUB_PROJECT_BASE + pathname);
  const bytes = await image.bytes();
  return bytes;
}

export default {
  async fetch(request, env, ctx) {
    const url = new URL(request.url);
    const pathname = url.pathname;
    // Switch based on the URL path
    switch (pathname) {
      // HTML ...
      case "/":
      case "/readme.html":
        return new Response(html_from_markup(readme), { headers: { 'Content-Type': 'text/html; charset=utf-8' }, });
      case "/theory.html":
        return new Response(html_from_markup(theory), { headers: { 'Content-Type': 'text/html; charset=utf-8' }, });
      case "/changelog.html":
        return new Response(html_from_markup(changelog), { headers: { 'Content-Type': 'text/html; charset=utf-8' }, });
      // CSS ...
      case "/simple.min.css":
        return new Response(simple_min_css, { headers: { 'Content-Type': 'text/css; charset=utf-8' }, });
      // IMAGES ...
      case "/kernel64logo-small.png":
      case "/kernel64logo-large.png":
      case "/kernel64logo-180x180.png":
        return new Response(await fetch_image_from_github(pathname), { headers: { 'Content-Type': 'image/png' }, });
      case "/favicon.ico":
        return new Response(await fetch_image_from_github(pathname), { headers: { 'Content-Type': 'image/x-icon' }, });
      // 404 ...
      default:
        let error404_markup = "";
        error404_markup += "# Oops! Page Not Found \n\n";
        error404_markup += "We're sorry; but, the page you are looking for doesn't exist, has been removed, or its name has changed.\n\n";
        error404_markup += "Request: **" + pathname + "** is incorrect\n\n";
        error404_markup += "[Return to home page](/)\n\n";
        // Return a 404 Page Not Found for unmatched paths
        return new Response(html_from_markup(error404_markup), { headers: { 'Content-Type': 'text/html; charset=utf-8' }, status: 404 });
    }
  },
};
