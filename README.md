# laptimer
Arduino laptimer for go kart

## Operation

Holding down `select` for two seconds open configuration mode where `up` or `right` increases the number of sectors by one and `down`or `left` decreses the number by one. Pressing `select` again stores the chosen value in flash and will be read and used as default after boot.

![Configuration](https://raw.githubusercontent.com/SeniorWizard/laptimer/master/laptimer_config.jpg)

After boot or confinguration changes the data is wiped and the laptimer goes into `WAIT` mode that is waiting for first passage of a magnet strip which indicates we are starting the first lap.

The display will differ dependent on wheter the tack only has one magnet strip and each passage corresponds to a lap or it has serveral (up to 3) where split times wil be shown.

## Single sector mode

![single sector](https://raw.githubusercontent.com/SeniorWizard/laptimer/master/laptimer_single_sector.jpg)

First line of the display shows:
* Last laptime
* Last difference 
* current status

Second line shows:
* best laptime
* current lap

On the image is shown the first 3 laps.

* Initially we are waiting for the start, status are `WAIT` and all times are undefined signaled with `*`
* When we cross the strip the first time the internal timer is started, and the status is changed to `RACE` and the lapnumer is now `1`
* When we pass the second time the first lap is completed. Both last lap and best lap is recorded as `10.95` and the lapcounter is incremented
* Next passage is done after `12.91` a slower time indcated by the difference of `+1.96`
* Next passage is done after `10.73` a new best by `-0.22` also indicated by the status as well as the display blinking

## Multi sector mode

![multi sector](https://raw.githubusercontent.com/SeniorWizard/laptimer/master/laptimer_multi_sector.jpg)

First line of the display shows:
* Last sector time
* last sector time compared to previous best 
* current status

Second line shows:
* current rolling laptime, that is last full lap with reference to the current sector. If we are on a track with 3 sectors and pass start-finish line it would be the sum of sector1+sector2+sector3 as expected. When we complete sector1 the new sector time will be used instead thus giving a pseudo laptime corresponding to secor1.
* last rolling lap time compared to previous best 
* current lap and current sector

On the image is shown 4 passages completing the second lap to illustrate the idea:
* first sector is completed in `18.42` `2.08` slower than the first passage
* second sector is completed in `19.78` `5.55` slower than the first passage, before we were `1.00` behind thus now we are `6.55` slower on the rolling lap
* third sector also slower, note that we are now more than 10 seconds slower indicated by `+9.99` as we pass the start-finish line to lap 3
* first sector is then completed in `14.40`, a new best for this sector by `-1.94`, while our previous slow sector 2 and 3 still makes our pseudo laptime slower by almos 10 seconds.


