import { Component, OnInit } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { LightGallery } from 'lightgallery/lightgallery';
import lgZoom from 'lightgallery/plugins/zoom';
import { MatStepper } from '@angular/material/stepper';

interface WeekDay {
  value: string;
  viewValue: string;
}

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  coverageForm!: FormGroup;
  durationForm!: FormGroup;
  optionsForNewTestamentOnly!: FormGroup;
  optionsForWholeBible!: FormGroup;
  optionsForOthers!: FormGroup;
  lightGallery!: LightGallery;

  previewUrl = ''

  catchUpDays: WeekDay[] = [
    {value: 'sunday', viewValue: 'Sunday'},
    {value: 'monday', viewValue: 'Monday'},
    {value: 'tuesday', viewValue: 'Tuesday'},
    {value: 'wednesday', viewValue: 'Wednesday'},
    {value: 'thursday', viewValue: 'Thursday'},
    {value: 'friday', viewValue: 'Friday'},
    {value: 'saturday', viewValue: 'Saturday'}
  ];

  weekDays: WeekDay[] = [
    {value: 'everyday', viewValue: "No, I'll read every day!"},
    {value: 'sunday', viewValue: 'Sunday'},
    {value: 'monday', viewValue: 'Monday'},
    {value: 'tuesday', viewValue: 'Tuesday'},
    {value: 'wednesday', viewValue: 'Wednesday'},
    {value: 'thursday', viewValue: 'Thursday'},
    {value: 'friday', viewValue: 'Friday'},
    {value: 'saturday', viewValue: 'Saturday'}
  ];

  galleryItems: LightGallery["galleryItems"] = [];

  settings = {
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

  constructor(private _fb: FormBuilder) {
  }

  ngOnInit() {
    this.coverageForm = this._fb.group({
      coverageType: ['', Validators.required]
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

  onStepChange(stepper: MatStepper) {
    if (stepper.selectedIndex == stepper.steps.length - 1) {
      this.galleryItems.length = 0;
      for (let i = 0; i < 12; i++) {
        let urlParam = this.getUrlParam(2022, 0, i);
        this.galleryItems.push({
          // The gallery does not support zooming SVG image.
          src: '/hello/img.png?' + urlParam,
          // SVG scales better than PNG.
          thumb: '/hello/img.svg?' + urlParam
        });
      }
      if (this.durationType() === 'two-years') {
        for (let i = 0; i < 12; i++) {
          let urlParam = this.getUrlParam(2022, 1, i);
          this.galleryItems.push({
            src: '/hello/img.png?' + urlParam,
            thumb: '/hello/img.svg?' + urlParam
          });
        }
      }
      this.lightGallery.refresh(this.galleryItems);
    }
  }

  getUrlParam(year: number, yearIndex: number, month: number) {
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
    param.append('y', '2022');
    param.append('yi', yearIndex.toString());
    param.append('i', String(month));
    return param.toString();
  }
}
