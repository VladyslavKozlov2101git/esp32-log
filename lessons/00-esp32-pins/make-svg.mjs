// Regenerates img/*.svg from the drawing code in pins.html: `node make-svg.mjs`
import { readFileSync, writeFileSync } from 'node:fs';
const dir = new URL('.', import.meta.url);
const html = readFileSync(new URL('pins.html', dir), 'utf8');
const js = html.split('<script>')[1].split('// --- page ---')[0];
const { buildPinout, buildTTP } = new Function(js + 'return {buildPinout,buildTTP}')();
const style = '<style>.lbl{fill:#5a676d;font:12px sans-serif}</style>';
const out = { 'esp32-pinout': buildPinout(-1, 'all'), 'ttp223b': buildTTP(-1) };
for (const [name, svg] of Object.entries(out)) {
  const [x, y, w, h] = svg.match(/viewBox="([^"]+)"/)[1].split(' ');
  writeFileSync(new URL(`img/${name}.svg`, dir), svg
    .replace('<svg ', '<svg xmlns="http://www.w3.org/2000/svg" ')
    .replace(/(<svg[^>]*>)/, `$1${style}<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="12" fill="#dde5e3"/>`));
  console.log('img/' + name + '.svg');
}
