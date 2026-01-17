# Data

Represents an individual record stored in the database.

Fields (typical):
- id: unique identifier (string or integral)
- vector: std::vector<float> of length dim
- metadata: optional nlohmann::json for arbitrary attributes

Behavior:
- Upsert uses id to replace existing records.
- Query returns matches that include the stored id and score, and may include metadata.

Serialization:
- When saving/loading, vectors are serialized and may use base64 encoding for binary data.
- Metadata, if present, is serialized via nlohmann::json.