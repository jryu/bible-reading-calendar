import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule, Routes } from '@angular/router';
import { BuilderComponent } from './builder/builder.component'
import { AboutComponent } from './about/about.component'

const routes: Routes = [
    { path: '', component: BuilderComponent },
    { path: 'about', component: AboutComponent }
];

@NgModule({
  declarations: [],
  imports: [
    CommonModule,
    RouterModule.forRoot(routes, { useHash: true })
  ],
  exports: [RouterModule]
})
export class AppRoutingModule { }
