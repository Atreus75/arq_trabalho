FROM node:22-alpine AS frontend-build
WORKDIR /app/frontend
COPY frontend/package*.json ./
RUN npm ci
COPY frontend/ ./
RUN npm run build

FROM gcc:14-bookworm
WORKDIR /app
COPY . .
COPY --from=frontend-build /app/web ./web
RUN make
EXPOSE 3001
CMD ["./bin/mic1-local", "--port", "3001", "--no-browser"]
