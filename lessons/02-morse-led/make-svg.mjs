// Regenerates img/*.svg from the drawing code in wiring.html: `node make-svg.mjs`
import { readFileSync, writeFileSync } from 'node:fs';
const dir = new URL('.', import.meta.url);
const html = readFileSync(new URL('wiring.html', dir), 'utf8');
const js = html.split('<script>')[1].split('// --- page ---')[0];
const { build } = new Function(js + 'return {build}')();
const style = '<style>.lbl{fill:#5a676d;font:12px sans-serif}.lbl-b{fill:#1b2327;font:600 13px sans-serif}</style>';
const out = { wiring: build('rgb'), 'wiring-ky034': build('ky034') };
for (const [name, svg] of Object.entries(out)) {
  const [, vb] = svg.match(/viewBox="([^"]+)"/); const [x, y, w, h] = vb.split(' ');
  writeFileSync(new URL(`img/${name}.svg`, dir), svg
    .replace('<svg ', '<svg xmlns="http://www.w3.org/2000/svg" ')
    .replace(/(<svg[^>]*>)/, `$1${style}<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="12" fill="#dde5e3"/>`));
  console.log('img/' + name + '.svg');
}
