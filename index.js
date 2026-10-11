import { marked } from 'marked';
import readme from './README.md'; // Wrangler supports bundling text assets

export default {
  async fetch(request, env, ctx) {
    // Convert markdown to HTML string
    const htmlContent = marked.parse(readme);

    const fullHtml = `
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Project Documentation</title>
        <!-- Clean GitHub-like CSS styling -->
        <link rel="stylesheet" href="https://cloudflare.com">
        <style>
          body {
            box-sizing: border-box;
            min-width: 200px;
            max-width: 980px;
            margin: 0 auto;
            padding: 45px;
          }
          @media (max-width: 767px) {
            body { padding: 15px; }
          }
        </style>
      </head>
      <body class="markdown-body">
        ${htmlContent}
      </body>
      </html>
    `;

    return new Response(fullHtml, {
      headers: { 'Content-Type': 'text/html;charset=UTF-8' },
    });
  },
};
