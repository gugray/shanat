import * as esbuild from "esbuild"
import * as server from "./src-server/server.js"
import { livereloadPlugin } from "@jgoz/esbuild-plugin-livereload"

const frontendPort = 8080;
const serverPort = 8090;

// const timestamp = (+new Date).toString(36);
//
// const args = (argList => {
//   let res = {};
//   let opt, thisOpt, curOpt;
//   for (let i = 0; i < argList.length; i++) {
//     thisOpt = argList[i].trim();
//     opt = thisOpt.replace(/^\-+/, "");
//     if (opt === thisOpt) {
//       // argument value
//       if (curOpt) res[curOpt] = opt;
//       curOpt = null;
//     } else {
//       // argument name
//       curOpt = opt;
//       res[curOpt] = true;
//     }
//   }
//   //console.log(res);
//   return res;
// })(process.argv);

async function runFrontend() {
  const entryPoints = [
    "src-frontend/index.html",
    "src-frontend/app.css",
    "src-frontend/app.js",
    "src-frontend/static/*",
  ];
  const plugins = [
    livereloadPlugin({port: 53000}),
  ];
  const context = await esbuild.context({
    entryPoints: entryPoints,
    outdir: "public",
    bundle: true,
    sourcemap: true,
    loader: {
      ".html": "copy",
      ".css": "copy",
      ".svg": "copy",
      ".woff2": "copy",
    },
    define: {
      global: 'window', // 👈 Polyfill for global
    },
    write: true,
    metafile: true,
    plugins: plugins,
  });

  await context.watch();
  await context.serve({
    port: frontendPort,
    servedir: "public",
    fallback: "public/index.html",
  });
  console.log(`Serving frontend on port ${frontendPort}`);

  // If only building
  // await context.rebuild();
  // await context.dispose();
  // process.exit(0);
}

void server.run(serverPort);
void runFrontend();
