FROM debian:bookworm-slim AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
        clang make ca-certificates && \
    rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN CC=clang make all && CC=clang make test

FROM python:3.11-slim AS final
WORKDIR /app
RUN useradd -m -u 1001 ss
COPY --from=build /src/build/sensorsim /usr/local/bin/sensorsim
COPY python/pyproject.toml python/README.md ./
COPY python/sensorsim ./sensorsim
RUN pip install --no-cache-dir -e .
USER ss
ENTRYPOINT ["sensorsim"]
