"""Bundle retained rendering failures into an offline, dependency-free gallery."""
import argparse
import base64
import html
from pathlib import Path
import shutil


def image_panel(source, destination, label):
    if not source.is_file():
        return f'<figure><figcaption>{label}</figcaption><p>Image unavailable</p></figure>'
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    data = base64.b64encode(source.read_bytes()).decode('ascii')
    return (f'<figure><figcaption>{label}</figcaption>'
            f'<img alt="{label}" src="data:image/png;base64,{data}"></figure>')


def build_report(results, references, output):
    output.mkdir(parents=True, exist_ok=True)
    sections = []
    # Matched images are removed by compareImages; remaining reference-backed
    # actual images therefore include mismatches and incomplete comparisons.
    actuals = sorted(path for path in results.rglob('*.png')
                     if not path.stem.endswith('_difference'))
    for actual in actuals:
        relative = actual.relative_to(results)
        expected = references / relative
        if not expected.is_file():
            continue  # Exclude comparator unit-test fixtures, which intentionally mismatch.
        difference = actual.with_name(actual.stem + '_difference.png')
        panels = ''.join(image_panel(source, output / label.lower() / relative, label)
                         for source, label in [(expected, 'Reference'),
                                               (actual, 'Actual'), (difference, 'Difference')])
        sections.append(f'<section><h2>{html.escape(relative.as_posix())}</h2>'
                        f'<div class="images">{panels}</div></section>')
    title = f'Visual comparison report — {len(sections)} retained images'
    document = '''<!doctype html><html lang="en"><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Visual comparison report</title><style>
body{font:16px system-ui;margin:24px;background:#17191d;color:#eee}
h1{font-size:24px}h2{font-size:17px;overflow-wrap:anywhere}
section{border-top:1px solid #555;padding:16px 0}.images{display:flex;gap:16px;overflow:auto}
figure{margin:0;flex:0 0 auto}figcaption{font-weight:bold;margin-bottom:8px}
img{display:block;max-width:none;image-rendering:pixelated;
background:repeating-conic-gradient(#ccc 0% 25%,#fff 0% 50%) 0/16px 16px}
p{max-width:85ch;line-height:1.5}</style><body>'''
    document += f'<h1>{html.escape(title)}</h1>'
    document += ('<p>Reference, actual render and difference are displayed at native resolution. '
                 'Scroll horizontally or use browser zoom to inspect pixels. Red pixels in the '
                 'difference mark mismatches under the existing comparison tolerances.</p>'
                 '<p>This HTML embeds its images and can be opened on its own. Separate PNGs '
                 'are also included in the reference, actual and difference folders.</p>')
    document += ''.join(sections) or ('<p>No retained reference-backed images were found. '
                                    'Check the test logs: tests may have passed or failed before rendering.</p>')
    (output / 'index.html').write_text(document + '</body></html>', encoding='utf-8')
    print(title)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results', type=Path, required=True)
    parser.add_argument('--references', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    build_report(args.results, args.references, args.output)
