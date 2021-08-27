import { Component, Inject, LOCALE_ID, OnInit, ViewChild } from '@angular/core';
import { Router } from '@angular/router';
import { MatSidenav } from '@angular/material/sidenav';

@Component({
  selector: 'app-root',
  templateUrl: './app.component.html',
  styleUrls: ['./app.component.css']
})
export class AppComponent implements OnInit {
  @ViewChild('sidenav') sidenav!: MatSidenav;

  constructor(private router: Router,
              @Inject(LOCALE_ID) public locale: string) { }

  ngOnInit() {
  }

  navigate(url: string) {
    this.router.navigateByUrl(url);
    this.sidenav.close();
  }
}
