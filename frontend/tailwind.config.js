/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      fontFamily: {
        // Fonte de sistema crua, como o Windows Classic usava (Tahoma/MS Sans Serif).
        // Sem depender de CDN: cai direto nas fontes de sistema do usuario.
        sans: ["Tahoma", "'MS Sans Serif'", "Geneva", "Verdana", "sans-serif"],
        mono: ["'Courier New'", "Consolas", "monospace"],
      },
      colors: {
        win: {
          face: "#C0C0C0",       // cinza de face padrao (janelas, botoes)
          faceDark: "#808080",   // sombra media do bevel
          faceDarker: "#404040", // sombra escura do bevel (borda externa)
          faceLight: "#DFDFDF",  // realce claro do bevel
          white: "#FFFFFF",      // realce mais claro (borda externa saliente)
          titleActive: "#000080",     // azul marinho da barra de titulo ativa
          titleActiveEnd: "#1084D0",  // fim do gradiente da barra de titulo
          titleInactive: "#808080",
          desktop: "#008080",    // teal classico do papel de parede
          highlight: "#000080",  // selecao (fundo azul)
          highlightText: "#FFFFFF",
          text: "#000000",
          disabledText: "#808080",
          link: "#0000FF",
        },
        // Cores de sinal para dados tecnicos (bits ativos, registradores),
        // adaptadas para contraste em fundo claro em vez do tema escuro anterior.
        signal: {
          data: "#000080",     // azul marinho: bit ligado / dado ativo
          control: "#800000",  // vinho: barramento/controle
          warn: "#804000",     // marrom-laranja: PC/atencao
          danger: "#FF0000",
        },
      },
      boxShadow: {
        // Bevel "saliente" (raised): usado em botoes, janelas, paineis no estado normal.
        "win-out": "inset -1px -1px 0 0 #404040, inset 1px 1px 0 0 #FFFFFF, inset -2px -2px 0 0 #808080, inset 2px 2px 0 0 #DFDFDF",
        // Bevel "entalhado" (sunken): usado em inputs, areas de texto, e botoes pressionados.
        "win-in": "inset 1px 1px 0 0 #404040, inset -1px -1px 0 0 #FFFFFF, inset 2px 2px 0 0 #808080, inset -2px -2px 0 0 #DFDFDF",
        // Bevel simples de 1px, usado em elementos menores (menu items, tabs).
        "win-out-1": "inset -1px -1px 0 0 #404040, inset 1px 1px 0 0 #FFFFFF",
        "win-in-1": "inset 1px 1px 0 0 #404040, inset -1px -1px 0 0 #FFFFFF",
      },
    },
  },
  plugins: [],
}
