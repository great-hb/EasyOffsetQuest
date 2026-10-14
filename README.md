# EasyOffsetQuest

EasyOffsetQuest was created primarily to bring PC EasyOffset controller settings to native Quest Beat Saber **1.40.8** (tested only on Quest 2). This mod has a much smaller feature set compared to PC EasyOffset.

Note: This mod was developed with AI.

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

## Building

On Windows, install **Python 3** and **CMake 3.22+**, then run:

```powershell
python tools/bootstrap.py
python tools/prepare_dependencies.py
.\scripts\build.ps1
```

The first setup downloads the build tools and dependencies. The finished mod is written to `dist/EasyOffsetQuest.qmod`.

## Credits

- [EasyOffset](https://github.com/Reezonate/EasyOffset) by **Reezonate** — PC preset format, offset behavior, and UI inspiration.
- [CongXinJian](https://github.com/lixiangwuxian/CongXinJian) by **lixiangwuxian / lxtend** — Quest mod foundation and gesture controls.

See [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) and [LICENSE-EasyOffset](LICENSE-EasyOffset) for attribution and license details.
