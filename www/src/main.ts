/* eslint-disable vue/multi-word-component-names */
/* eslint-disable vue/no-reserved-component-names */
import 'primeicons/primeicons.css'
import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import PrimeVue from 'primevue/config'
import Aura from '@primevue/themes/aura';
import Card from 'primevue/card'
import ProgressSpinner from 'primevue/progressspinner';
import Button from 'primevue/button'
import App from './App.vue'

const app = createApp(App)
app.use(PrimeVue, { theme: {
      preset: Aura,
      options: {
        darkModeSelector: false
      }
  }
});

app.use(createPinia())
app.component('Card', Card)
app.component('ProgressSpinner', ProgressSpinner)
app.component('Button', Button)

app.mount('#app')
