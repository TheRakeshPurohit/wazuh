from typing import List

from pydantic import BaseModel

from wazuh.core.engine.models.events import StatelessEvent
from wazuh.core.indexer.models.events import AgentMetadata, Header, StatefulEvent, TaskResult


class StatefulEvents(BaseModel):
    agent_metadata: AgentMetadata
    headers: List[Header]
    events: List[StatefulEvent]


class StatefulEventsResponse(BaseModel):
    results: List[TaskResult]


class StatelessEvents(BaseModel):
    events: List[StatelessEvent]
