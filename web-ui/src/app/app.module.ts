import { NgModule } from '@angular/core';
import { BrowserModule } from '@angular/platform-browser';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';

import { AppComponent } from './app.component';
import { BrowserAnimationsModule } from '@angular/platform-browser/animations';
import { MatButtonModule } from '@angular/material/button';
import { MatDatepickerModule } from '@angular/material/datepicker';
import { MatDividerModule } from '@angular/material/divider';
import { MatFormFieldModule } from '@angular/material/form-field';
import { MatIconModule } from '@angular/material/icon';
import { MatInputModule } from '@angular/material/input';
import { MatListModule } from '@angular/material/list';
import { MatMenuModule } from '@angular/material/menu';
import { MatNativeDateModule } from '@angular/material/core';
import { MatRadioModule } from '@angular/material/radio';
import { MatSelectModule } from '@angular/material/select';
import { MatSidenavModule } from '@angular/material/sidenav';
import { MatSnackBarModule } from '@angular/material/snack-bar';
import { ClipboardModule } from '@angular/cdk/clipboard';
import { MatStepperModule } from '@angular/material/stepper';
import { MatToolbarModule } from '@angular/material/toolbar';

import { LightgalleryModule } from 'lightgallery/angular';
import { ShareButtonsModule } from 'ngx-sharebuttons/buttons';
import { ShareIconsModule } from 'ngx-sharebuttons/icons';

import { AppRoutingModule } from './app-routing.module';
import { AboutComponent } from './about/about.component';
import { BuilderComponent } from './builder/builder.component';

@NgModule({
  declarations: [
    AppComponent,
    BuilderComponent,
    AboutComponent
  ],
  imports: [
    BrowserModule,
    BrowserAnimationsModule,
    FormsModule,
    LightgalleryModule,
    ShareButtonsModule,
    ShareIconsModule,
    MatButtonModule,
    MatDatepickerModule,
    MatDividerModule,
    MatIconModule,
    MatInputModule,
    MatListModule,
    MatFormFieldModule,
    MatMenuModule,
    MatNativeDateModule,
    MatRadioModule,
    MatSelectModule,
    MatSidenavModule,
    MatSnackBarModule,
    MatStepperModule,
    MatToolbarModule,
    ReactiveFormsModule,
    AppRoutingModule,
    ClipboardModule
  ],
  providers: [],
  bootstrap: [AppComponent]
})
export class AppModule { }
