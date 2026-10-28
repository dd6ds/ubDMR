# Contributing to ubDMR

These rules keep the code readable for the people who review it — the
OpenStore reviewers and anyone else reading the repository. They apply to
every change, whether a person or a coding assistant wrote it.

A change that does not follow them is not finished.

Code under `third_party/` is kept identical to its upstream and is exempt
from these rules; update it as a whole and note the version in its README.

## 1. Only code that is used

- Everything in the repository is built and used by the app. No stubs, no
  placeholder pages, no directories "for later", no code behind a switch
  that is never turned on.
- Remove code that becomes unused in the same change. Git keeps the history;
  no `.bak`, `.old` or `.obsolete` copies.
- No unused public API: a `Q_INVOKABLE`, property or signal that nothing in
  QML or C++ uses is deleted.

## 2. A tidy repository

The top level holds only what building and understanding the app needs:
`README.md`, `CONTRIBUTING.md`, `LICENSE`, `CLAUDE.md`, the build files,
the click metadata and `third_party/`.

Not committed:

- Personal notes, to-do lists, "what I changed today" files and generated
  summaries. That information goes in the commit message.
- One-off fix or debug scripts (`fix-*.sh`, `DO_THIS.txt`, …). If a script is
  worth keeping, it gets a clear name, a comment saying what it does, and a
  mention in the README.
- Passwords, device IPs, personal paths such as `/home/<name>/…`, logs, build
  output.

## 3. File and function size

- **One page per QML file.** `main.qml` holds the window, the shared state and
  the page stack, nothing else.
- **One class per C++ file pair** (small helper structs may share a header).
  If a class grows past about 800 lines, split it by topic as
  `radiocontroller.cpp` / `radiocontroller_tx.cpp` do, with a comment at
  the top of each file that says what lives there.
- QML files over about 400 lines are split into components.
- Functions should fit on a screen (about 50 lines). Longer ones are split
  into named helpers.
- Lambdas in `connect()` stay short (a few lines). Anything longer becomes a
  named member function, so the constructor reads like a list of connections.

## 4. Names instead of magic values

- Page positions, timeouts, limits and protocol codes get a name
  (`app.pageRadio`, `kKeepAliveMs`, `TypeSuperHeader`).
  `mainStack.currentIndex = 3` is not allowed.
- Names say what something is: `tgField`, `onLoggedIn()`, not
  `input2`, `handle()`.

## 5. Comments

- Comments explain **why**, or what is not obvious from the code (protocol
  details, platform quirks, firmware behaviour). They do not repeat the code.
- Comments describe the code as it is now. History ("used to", "the old
  button", "fixed the bug where…") belongs in the commit message.
- Every file starts with a short comment saying what it is for.
- English, plain sentences.

## 6. Style

- Follow the code around you: 4-space indentation, Qt naming (`m_member`,
  `camelCase`, `kConstant`), `QStringLiteral` for literals in C++.
- Keep lines under 100 characters.
- No leftover debug output: no `console.log` in QML. `qDebug()` in C++ only
  where it helps on a device, prefixed with the class name
  (`"RewindClient: ..."`).

## 7. Platform rules

These are explained in the README ("Platform constraints"); the short form:

- Qt 5.12 only: `import QtQuick 2.12`, `QtQuick.Controls 2.12`,
  `Connections { onSignal: … }`.
- Use `AppLabel` / `AppField`, not bare `Label` / `TextField`.
- Do not set `QT_QPA_PLATFORM`, `MIR_SOCKET` or plugin paths in code.
- **Permissions:** `ubdmr.apparmor` lists only policy groups the app
  really uses. Adding one needs a reason in the README.
- `manifest.json` carries the real maintainer name and a reachable address.

## 8. Translations

- Every user-visible text is wrapped in `qsTr()` / `tr()`, with placeholders
  (`qsTr("Open %1").arg(name)`) instead of joined fragments.
- After changing texts, refresh all `i18n/*.ts` files as described in the
  README ("Translations") and translate every new entry; no entry may stay
  `unfinished`.

## 9. Before you commit

- [ ] `clickable build` succeeds without new compiler or QML warnings.
- [ ] `click-review` shows no errors.
- [ ] The app starts on the phone and the changed feature works.
- [ ] Nothing unused, no debug output, no notes or scripts added (rules 1–2).
- [ ] README updated if features, layout or build steps changed.
- [ ] One topic per commit, with a message that says what changed and why.
