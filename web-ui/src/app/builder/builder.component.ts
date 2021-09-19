import { Component, Inject, LOCALE_ID, OnInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';

import { MatDatepicker } from '@angular/material/datepicker';
import { MatSnackBar } from '@angular/material/snack-bar';
import { MatStepper } from '@angular/material/stepper';

import { Clipboard } from '@angular/cdk/clipboard';

import { LightGallery } from 'lightgallery/lightgallery';
import lgZoom from 'lightgallery/plugins/zoom';

interface WeekDay {
  value: string;
  viewValue: string;
}

@Component({
  selector: 'app-builder',
  templateUrl: './builder.component.html',
  styleUrls: ['./builder.component.sass']
})
export class BuilderComponent implements OnInit {
  coverageForm!: FormGroup;
  startDateForm!: FormGroup;
  durationForm!: FormGroup;
  optionsForNewTestamentOnly!: FormGroup;
  optionsForWholeBible!: FormGroup;
  optionsForOthers!: FormGroup;
  lightGallery!: LightGallery;

  catchUpDays: WeekDay[] = [
    {value: 'sunday', viewValue: $localize`Sunday`},
    {value: 'monday', viewValue: $localize`Monday`},
    {value: 'tuesday', viewValue: $localize`Tuesday`},
    {value: 'wednesday', viewValue: $localize`Wednesday`},
    {value: 'thursday', viewValue: $localize`Thursday`},
    {value: 'friday', viewValue: $localize`Friday`},
    {value: 'saturday', viewValue: $localize`Saturday`}
  ];

  weekDays: WeekDay[] = [
    {value: 'everyday', viewValue: $localize`No, I'll read every day!`},
    {value: 'sunday', viewValue: $localize`Sunday`},
    {value: 'monday', viewValue: $localize`Monday`},
    {value: 'tuesday', viewValue: $localize`Tuesday`},
    {value: 'wednesday', viewValue: $localize`Wednesday`},
    {value: 'thursday', viewValue: $localize`Thursday`},
    {value: 'friday', viewValue: $localize`Friday`},
    {value: 'saturday', viewValue: $localize`Saturday`}
  ];

  galleryItems: LightGallery["galleryItems"] = [];
  galleryItemsByYear = new Map();

  pdfUrl = '';

  settings = {
    allowMediaOverlap: true,
    download: false,
    dynamic: true,
    dynamicEl: [],
    loop: false,
    mobileSettings: {
      controls: true,
      download: false,
      showCloseIcon: true,
    },
    plugins: [lgZoom],
  };

  customStartDate = new Date();
  today = this.customStartDate;

  minStartDate = new Date(
    this.customStartDate.getFullYear() - 2,
    this.customStartDate.getMonth(),
    this.customStartDate.getDate());

  maxStartDate = new Date(
    this.customStartDate.getFullYear() +
      (this.customStartDate.getMonth() == 1 &&
       this.customStartDate.getDate() == 1 ? 0 : 1),
    0, 1);

  constructor(private _fb: FormBuilder,
              private _snackBar: MatSnackBar,
              private _clipboard: Clipboard,
              @Inject(LOCALE_ID) public locale: string) {
  }

  ngOnInit(): void {
    this.coverageForm = this._fb.group({
      coverageType: ['', Validators.required]
    });
    this.startDateForm = this._fb.group({
      startDateType: ['', Validators.required]
    });
    this.durationForm = this._fb.group({
      durationType: ['', Validators.required]
    });

    this.optionsForNewTestamentOnly = this._fb.group({
      restDay1: ['', Validators.required],
      restDay2: ['', Validators.required],
    });

    this.optionsForWholeBible = this._fb.group({
      orderType: ['', Validators.required],
      restDay: ['', Validators.required],
    });

    this.optionsForOthers = this._fb.group({
      restDay: ['', Validators.required],
    });

    this.coverageForm.patchValue({coverageType: 'new-testament'});
    this.startDateForm.patchValue({startDateType: 'today'});
    this.durationForm.patchValue({durationType: 'one-year'});

    this.optionsForNewTestamentOnly.patchValue({
      restDay1: 'sunday',
      restDay2: 'saturday'
    });

    this.optionsForWholeBible.patchValue({
      orderType: 'old-testament-first',
      restDay: 'everyday'
    });

    this.optionsForOthers.patchValue({
      restDay: 'everyday'
    });
  }

  onInit = (detail: any): void => {
    this.lightGallery = detail.instance;
  };

  coverageType() {
    return this.coverageForm.get('coverageType')!.value;
  }

  durationType() {
    if (['old-testament', 'whole-bible'].includes(
      this.coverageType())) {
      return this.durationForm.get('durationType')!.value;
    } else {
      return '';
    }
  }

  startDateType() {
    return this.startDateForm.get('startDateType')!.value;
  }

  getStartDate() {
    let startDate = new Date(this.today.getTime());
    switch (this.startDateType()) {
      case 'tomorrow':
        startDate.setDate(startDate.getDate() + 1);
        break;
      case 'next-month':
        startDate.setDate(1);
        startDate.setMonth(startDate.getMonth() + 1);
        break;
      case 'next-year':
        startDate.setDate(1);
        startDate.setMonth(0);
        startDate.setFullYear(startDate.getFullYear() + 1);
        break;
      case 'custom':
        return new Date(this.customStartDate.getTime());
    }
    return startDate;
  }

  getStartDateParam() {
    let startDate = this.getStartDate();
    var y = startDate.getFullYear();
    var m = startDate.getMonth() + 1;
    var d = startDate.getDate();
    var mm = m < 10 ? '0' + m : m;
    var dd = d < 10 ? '0' + d : d;
    return '' + y + mm + dd;
  }

  getCustomStartDate() {
    if (this.customStartDate.getTime() == this.today.getTime()) {
      return '';
    }
    if (this.locale == 'ko') {
      return this.customStartDate.getFullYear() + '년 ' +
        (this.customStartDate.getMonth() + 1) + '월 ' +
        this.customStartDate.getDate() + '일';
    }
    return this.customStartDate.toLocaleDateString('en-US');
  }

  onStepChange(stepper: MatStepper) {
    if (stepper.selectedIndex == stepper.steps.length - 2) {
      // "Preview" is selected.
      this.galleryItems.length = 0;
      this.galleryItemsByYear.clear();

      let d = this.getStartDate();

      let totalMonths = 12;
      if (this.durationType() === 'two-years') {
        totalMonths += 12;
      }
      if (d.getDate() > 1) {
        totalMonths += 1;
      }

      // In case the date is on 31, adding a month to the date may skip months
      // which doesn't have 31 days.
      d.setDate(1);

      for (let i = 0; i < totalMonths; i++) {
        let url = '/cpp/img.png?' + this.getUrlParam(d);

        this.galleryItems.push({
          src: url,
        });

        if (!this.galleryItemsByYear.has(d.getFullYear())) {
          this.galleryItemsByYear.set(d.getFullYear(), []);
        }
        this.galleryItemsByYear.get(d.getFullYear()).push({
          index: i,
          src: url
        });

        d.setMonth(d.getMonth() + 1);
      }
      this.lightGallery.refresh(this.galleryItems);
    } else if (stepper.selectedIndex == stepper.steps.length - 1) {
      // "Download" is selected
      // TODO: Remove unnecessary month param.
      this.pdfUrl = '/cpp/img.pdf?' + this.getUrlParam(this.getStartDate());
    }
  }

  getUrlParam(startDate: Date) {
    var param = new URLSearchParams();
    param.append('c', this.coverageType());

    switch (this.coverageType()) {
      case 'new-testament':
        param.append('r1',
                     this.optionsForNewTestamentOnly.get('restDay1')!.value);
        param.append('r2',
                     this.optionsForNewTestamentOnly.get('restDay2')!.value);
        break;

      case 'old-testament':
        param.append('d', this.durationType());
        param.append('r',
                     this.optionsForOthers.get('restDay')!.value);
        break;

      case 'whole-bible':
        param.append('d', this.durationType());
        param.append('o',
                     this.optionsForWholeBible.get('orderType')!.value);
        param.append('r',
                     this.optionsForWholeBible.get('restDay')!.value);
        break;

      case 'new-testament-and-psalms':
        param.append('r',
                     this.optionsForOthers.get('restDay')!.value);
        break;
    }
    param.append('y', String(startDate.getFullYear()));
    param.append('m', String(startDate.getMonth() + 1));
    param.append('l', this.locale);
    param.append('s', this.getStartDateParam());
    return param.toString();
  }

  copyCalendarLink() {
    this._clipboard.copy('http://biblereadingcalendar.com/cpp/c.ics?' +
                         this.getUrlParam(this.getStartDate()));

    this._snackBar.open($localize`Link is copied to clipboard`, '',
                        {duration: 3000});
  }
}
