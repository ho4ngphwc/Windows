FROM node:18-alpine 

WORKDIR /app 
COPY server.js . 

RUN npm install init -y && npm install express 

EXPOSE 8000 
CMD ["node", "server.js"]

