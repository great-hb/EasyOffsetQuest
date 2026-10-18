# EasyOffsetQuest

EasyOffsetQuest was created primarily to bring [PC EasyOffset](https://github.com/Reezonate/EasyOffset) controller settings to native Quest Beat Saber **1.40.8** (tested on Quest 2).

Note: This mod was developed almost entirely with AI. I know how to code, but I'm not trying to spend hours learning how to create mods on quest when all I want is my controller offsets. Sue me.

## PC features not included

- Swing Benchmark measurements and swing-based correction tools
- Curve/Balance sliders and reference controls
- Undo/redo history
- Room Offset adjustment
- Import from base-game controller settings and Universal Import from other mods

## Installation

Install `EasyOffsetQuest.qmod` using [ModsBeforeFriday (MBF)](https://mbf.bsquest.xyz/). Open the **Easy Offset** tab on the song selection screen to adjust your controllers.

## Loading PC presets

1. Save a preset in PC **Easy Offset 2.1.2 or newer**
2. Open your PC Beat Saber folder and find the saved preset's `.json` file in:

   ```text
   UserData/EasyOffset/Presets/
   ```

3. Copy that file into:

   ```text
   /sdcard/ModData/com.beatgames.beatsaber/Mods/EasyOffsetQuest/Presets/
   ```

4. In the Easy Offset tab, press **Refresh**, select the preset, and press **Load**.

Use **PC Meta Link** as the profile tracking space for PC presets. 

- **Save** updates the selected preset.
- **Save As** creates a new named preset from the entered name.
- **Delete** removes the selected preset after confirmation; 
 Default is kept as the fallback preset.

Controller adjustments are also saved automatically between launches.

## Images

<img width="1988" height="1118" alt="EasyOffsetQuest1" src="https://github.com/user-attachments/assets/dcf00ddb-d2be-419c-9fa6-2bc9c737ea97" />
<img width="1988" height="679" alt="EasyOffsetQuest2-" src="https://github.com/user-attachments/assets/f65cb4e0-d2ed-455b-adfc-8ce4266d7ed3" />

## Building

On Windows, install **Python 3** and **CMake 3.22+**, then run:

```powershell
python tools/bootstrap.py
python tools/prepare_dependencies.py
.\scripts\build.ps1
```

The first setup downloads the build tools and dependencies. The finished mod is written to `dist/EasyOffsetQuest.qmod`.

## GitHub builds

GitHub Actions builds the mod on pushes to `main`, pull requests, and manual runs from **Actions → Build QMOD**. Successful runs provide a QMOD, checksums, and a build report under **Artifacts**. No extra repository secrets are needed.

For a release, keep the version in `mod.json`, `qpm.json`, `qpm.shared.json`, and `CMakeLists.txt` consistent, then push the matching `v<version>` tag. CI builds the QMOD and attaches it to a **draft** release. Download the Actions artifact, install its QMOD through MBF, and test it in-game before publishing the draft. Automated checks validate MBF's manifest schema, package contents, ARM64 library, and offset math; they do not run MBF or Beat Saber.

The workflow follows the QPM/NDK build pattern used by [Quest-BSML](https://github.com/bsq-ports/Quest-BSML/blob/master/.github/workflows/build-ndk.yml) and [custom-types](https://github.com/QuestPackageManager/Il2CppQuestTypePatching/blob/master/.github/workflows/build-ndk.yml).

## Credits

- [EasyOffset](https://github.com/Reezonate/EasyOffset) by **Reezonate** — PC preset format, offset behavior, and UI inspiration.
- [CongXinJian](https://github.com/lixiangwuxian/CongXinJian) by **lixiangwuxian / lxtend** — Quest mod foundation and gesture controls.

See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) and [LICENSE-EasyOffset](LICENSE-EasyOffset) for attribution and license details.
