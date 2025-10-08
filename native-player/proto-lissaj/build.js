import * as esbuild from "esbuild"
import glsl from "./glsl-plugin.js"
import { livereloadPlugin } from "@jgoz/esbuild-plugin-livereload"

async function build() {

  const entryPoints = [
    "src/index.html",
    "src/app.js",
    "src/app.css",
  ];

  const plugins = [
    livereloadPlugin({port: 53000}),
    glsl(),
  ];

  const context = await esbuild.context({
    entryPoints: entryPoints,
    outdir: "public",
    bundle: true,
    sourcemap: true,
    format: 'esm',
    loader: {
      ".html": "copy",
      ".css": "copy",
      ".json": "copy",
      ".bin": "copy",
      ".jpg": "copy",
    },
    write: true,
    metafile: true,
    plugins: plugins,
  });

  await context.watch();
  await context.serve({port: 8080});
  console.log("Hello! Listening at http://localhost:8080/");
}

void build();
