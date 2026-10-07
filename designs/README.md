# Design studies

Open these HTML files locally after cloning. Assets use relative paths; no web
server, external fonts or JavaScript dependencies are required.

| Page | Designs |
|---|---|
| [Pacific Relay](pacific-relay-designs.html) | ORBIT ATLAS, PACIFIC HORIZON, MERIDIAN |
| [Across the Pacific](pacific-new-studies.html) | PACIFIC PRESS, GROVE STUDY, NIGHT FREQUENCY |
| [Pacific Press refinement](pacific-press-refined.html) | Original and white-background editions, including the ocean-navy primary text |

The pages share fixed UTC examples and provide scale, Japanese day/night,
PST/PDT and monochrome controls. The approved native faces are ORBIT ATLAS and
PACIFIC PRESS; the other directions remain browser studies.

ImageGen artwork and prompts are in `assets/`. The Earth was generated and then
simplified using the built-in ImageGen tool. Its source is
`assets/pacific-earth-source-v2.png`; the generation and refinement prompts are
`earth-generation-prompt.txt` and `earth-refinement-prompt.txt`. Native Earth
exports use 13 colors plus transparency in indexed PNGs at 124×124 and 82×82.
The wave, forest and radio originals and their prompts are in
`assets/new-studies/`; the white wave's edit prompt is retained there too.

Native build resources are kept separately in `resources/` at the repository
root. Their PNGs already use the supported Emery palette, so building does not
require generating or converting artwork again. Browser font metrics differ
slightly from the DejaVu Sans Bold subsets used on the watch.
