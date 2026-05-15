# MarkdownViewer

A standalone macOS markdown viewer with live file reload. Opens a `.md` file, renders it, and re-renders automatically on every save.

![MarkdownViewer rendering a markdown file](./docs/screenshot.png)

Fork of [sbarex/QLMarkdown](https://github.com/sbarex/QLMarkdown), trimmed down from a Quick Look extension into a plain app.

## Build

Requires Xcode 26 and `cmake` (for the bundled cmark-gfm build).

```sh
xcodebuild -project MarkdownViewer.xcodeproj \
  -scheme MarkdownViewer \
  -configuration Release \
  CODE_SIGN_IDENTITY="-" CODE_SIGN_STYLE=Manual \
  ONLY_ACTIVE_ARCH=YES build
```

The product lands in `~/Library/Developer/Xcode/DerivedData/MarkdownViewer-*/Build/Products/Release/MarkdownViewer.app`. Drop it in `/Applications`, then `codesign --force --sign - --deep` it for local launch.

## Open a file

- Finder: double-click or right-click → Open With
- Terminal: `open -a MarkdownViewer file.md`
- Drag-and-drop onto the window

## License

GPL-3.0 — see [`LICENSE`](./LICENSE).

The Swift code in this repository started as a derivative of [sbarex/QLMarkdown](https://github.com/sbarex/QLMarkdown) (MIT), which the MIT license allows us to relicense. The combined work is GPL-3.0 because the syntax-highlighting library it links against ([saalen/highlight](https://gitlab.com/saalen/highlight)) is GPL-3.0.
