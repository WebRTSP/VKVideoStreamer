/* eslint-disable vue/multi-word-component-names */
/* eslint-disable vue/no-reserved-component-names */
import './assets/main.css'

import { createApp } from 'vue'
import { createPinia } from 'pinia'
import PrimeVue from 'primevue/config'
import Aura from '@primevue/themes/aura';
import Card from 'primevue/card'
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

app.mount('#app')
