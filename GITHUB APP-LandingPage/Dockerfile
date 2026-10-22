FROM nginx:1.27-alpine

# Copiamos el sitio estático al directorio de Nginx
COPY . /usr/share/nginx/html

# Configuración de Nginx para servir la landing
COPY nginx.conf /etc/nginx/conf.d/default.conf

