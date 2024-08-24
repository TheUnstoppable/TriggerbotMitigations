# TriggerbotMitigations
[![Jenkins Build](https://img.shields.io/jenkins/build?jobUrl=https%3A%2F%2Fci.unstoppable.work%2Fjob%2FSSGM%2520Plugins%2Fjob%2FTriggerbotMitigations%2F)](https://ci.unstoppable.work/job/SSGM%20Plugins/job/TriggerbotMitigations/)
[![Discord](https://img.shields.io/discord/647431164138749966?label=support)](https://discord.gg/KjeQ7xv)
[![Buy me a Coffee](https://img.shields.io/badge/buy%20me%20a%20coffee-yellow)](https://buymeacoffee.com/theunstoppable)

## Overview
TriggerbotMitigations is an SSGM plugin (compatible with Dragonade) that aims to stop generic and some memory-based trigger bots using offered engine functions.

## Installation
- Grab the plugin files from [Jenkins](https://ci.unstoppable.work/job/SSGM%20Plugins/job/TriggerbotMitigations/), and extract the file into the server folder.
- Add the plugin name under \[Plugins\] section.
  - For SSGM: `99=TriggerbotMitigations.dll` (replace 99 with the next number in the list)
  - For Dragonade: `TriggerbotMitigations.dll=1`
- Add the following lines in either ssgm.ini (for SSGM) or da.ini (for Dragonade).
   ```
   ; MitigationType=
   ; 
   ;  The default mitigation type to apply to all players. Valid values are:
   ;  "Off": Does not change anything.
   ;  "Passive": Slightly changes the enemy reticle color per-player.
   ;  "Low": Replaces enemy unit reticle color with friendly unit reticle color.
   ;  "Medium": Replaces enemy unit and friendly unit reticle colors with neutral reticle color.
   ;  "High": Replaces friendly unit and neutral reticle colors with enemy unit reticle color.
   ;  "Aggressive": Disables targeting objects except for buildings and pokable objects.
   
   MitigationType=Off
   
   
   ; ColorThreshold=
   ; 
   ;  Minimum and maximum color threshold values while changing the enemy reticle color for players
   ;  with "Passive" mitigation level, in the format of "Minimum-Maximum".
   
   ColorThreshold=20-50
   ```
- All done!

## Usage
Mitigation type can be changed globally (see [Installation](#installation) section), or per-player (named Mitigation Exceptions) using console commands.
The plugin automatically drops and maintains a file named `MitigationExceptions.ini` to override the global setting and apply mitigation exceptions for players included in the file.

### Console Commands
- `addmitigationexception <player id> <level>` adds a mitigation exception for specified player. `<level>` is a value between 0 - 5, where 0 is "Off" and 5 is "Aggressive".
- `removemitigationexception <player id>` removes the mitigation exception for specified player.
- `flushmitigationexceptions` ensures that all mitigation exceptions are written to the file.

## Support
You will get the fastest response by joining in Discord server at https://discord.gg/KjeQ7xv, but be sure to read `#important` channel first.  
Feel free to join to suggest changes and request help.  
